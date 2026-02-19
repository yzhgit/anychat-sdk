#include "database.h"
#include "migrations.h"

#include <sqlite3.h>

#include <cassert>
#include <condition_variable>
#include <future>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace anychat::db {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Bind a single DbValue to a prepared statement at the given 1-based index.
static bool bindValue(sqlite3_stmt* stmt, int idx, const DbValue& val) {
    return std::visit([&](auto&& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        int rc = SQLITE_OK;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            rc = sqlite3_bind_null(stmt, idx);
        } else if constexpr (std::is_same_v<T, int64_t>) {
            rc = sqlite3_bind_int64(stmt, idx, v);
        } else if constexpr (std::is_same_v<T, double>) {
            rc = sqlite3_bind_double(stmt, idx, v);
        } else if constexpr (std::is_same_v<T, std::string>) {
            rc = sqlite3_bind_text(stmt, idx, v.c_str(),
                                   static_cast<int>(v.size()), SQLITE_TRANSIENT);
        }
        return rc == SQLITE_OK;
    }, val);
}

// Bind all params to a prepared statement.
static bool bindParams(sqlite3_stmt* stmt, const Params& params) {
    for (int i = 0; i < static_cast<int>(params.size()); ++i) {
        if (!bindValue(stmt, i + 1, params[i])) return false;
    }
    return true;
}

// Step a prepared statement to completion and collect all rows.
// Returns an error string on failure (empty on success).
static std::string stepAll(sqlite3_stmt* stmt, Rows& out) {
    int cols = sqlite3_column_count(stmt);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Row row;
        for (int c = 0; c < cols; ++c) {
            const char* name = sqlite3_column_name(stmt, c);
            const char* text = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, c));
            row[name ? name : ""] = text ? text : "";
        }
        out.push_back(std::move(row));
    }
    if (rc != SQLITE_DONE) {
        return sqlite3_errmsg(sqlite3_db_handle(stmt));
    }
    return {};
}

// Prepare + bind + stepAll, then finalize.  Returns error string or empty.
static std::string execOrQuery(sqlite3* db,
                               const std::string& sql,
                               const Params& params,
                               Rows* out /* nullptr = exec-only */) {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return sqlite3_errmsg(db);
    }
    if (!bindParams(stmt, params)) {
        std::string err = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return err;
    }
    Rows dummy;
    Rows& rows = out ? *out : dummy;
    std::string err = stepAll(stmt, rows);
    sqlite3_finalize(stmt);
    return err;
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct Database::Impl {
    sqlite3*    db      = nullptr;
    std::string path;
    bool        open_   = false;

    std::thread              worker;
    std::queue<std::function<void()>> tasks;
    std::mutex               mu;
    std::condition_variable  cv;
    bool                     stopping = false;

    void start() {
        worker = std::thread([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lk(mu);
                    cv.wait(lk, [this] { return stopping || !tasks.empty(); });
                    if (stopping && tasks.empty()) break;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lk(mu);
            stopping = true;
        }
        cv.notify_all();
        if (worker.joinable()) worker.join();
    }

    // Post a task to the worker queue (fire-and-forget).
    template<typename F>
    void post(F&& f) {
        {
            std::lock_guard<std::mutex> lk(mu);
            tasks.push(std::forward<F>(f));
        }
        cv.notify_one();
    }

    // Post a task and block until it returns (sync helper).
    // R must be default-constructible or we use promise<R>.
    template<typename F>
    auto postSync(F&& f) -> decltype(f()) {
        using R = decltype(f());
        std::promise<R> p;
        auto fut = p.get_future();
        post([fn = std::forward<F>(f), &p]() mutable {
            if constexpr (std::is_void_v<R>) {
                fn();
                p.set_value();
            } else {
                p.set_value(fn());
            }
        });
        return fut.get();
    }
};

// ---------------------------------------------------------------------------
// Database public API
// ---------------------------------------------------------------------------

Database::Database(std::string path)
    : impl_(std::make_unique<Impl>()) {
    impl_->path = std::move(path);
}

Database::~Database() {
    close();
}

bool Database::open() {
    // open() is called from outside before any other use; start the worker.
    impl_->start();

    return impl_->postSync([this]() -> bool {
        int rc = sqlite3_open(impl_->path.c_str(), &impl_->db);
        if (rc != SQLITE_OK) {
            sqlite3_close(impl_->db);
            impl_->db = nullptr;
            return false;
        }

        // Enable WAL mode for better concurrent read performance.
        sqlite3_exec(impl_->db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        // Reasonable busy timeout (5 s).
        sqlite3_busy_timeout(impl_->db, 5000);
        // Foreign-key enforcement.
        sqlite3_exec(impl_->db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

        if (!runMigrations(impl_->db)) {
            sqlite3_close(impl_->db);
            impl_->db = nullptr;
            return false;
        }

        impl_->open_ = true;
        return true;
    });
}

void Database::close() {
    if (!impl_) return;
    // Signal worker to stop after draining the queue.
    impl_->stop();

    if (impl_->db) {
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
    }
    impl_->open_ = false;
}

// ---------------------------------------------------------------------------
// Async variants
// ---------------------------------------------------------------------------

void Database::exec(std::string sql, Params params, ExecCallback cb) {
    impl_->post([this, sql = std::move(sql), params = std::move(params),
                 cb = std::move(cb)]() mutable {
        std::string err = execOrQuery(impl_->db, sql, params, nullptr);
        if (cb) cb(err.empty(), err);
    });
}

void Database::query(std::string sql, Params params, QueryCallback cb) {
    impl_->post([this, sql = std::move(sql), params = std::move(params),
                 cb = std::move(cb)]() mutable {
        Rows rows;
        std::string err = execOrQuery(impl_->db, sql, params, &rows);
        if (cb) cb(std::move(rows), err);
    });
}

// ---------------------------------------------------------------------------
// Sync variants
// ---------------------------------------------------------------------------

bool Database::execSync(const std::string& sql, Params params) {
    return impl_->postSync([this, &sql, &params]() -> bool {
        std::string err = execOrQuery(impl_->db, sql, params, nullptr);
        return err.empty();
    });
}

Rows Database::querySync(const std::string& sql, Params params) {
    return impl_->postSync([this, &sql, &params]() -> Rows {
        Rows rows;
        execOrQuery(impl_->db, sql, params, &rows);
        return rows;
    });
}

// ---------------------------------------------------------------------------
// Metadata helpers
// ---------------------------------------------------------------------------

std::string Database::getMeta(const std::string& key,
                               const std::string& default_val) {
    Rows rows = querySync("SELECT value FROM metadata WHERE key = ?",
                          {key});
    if (rows.empty()) return default_val;
    auto it = rows[0].find("value");
    if (it == rows[0].end()) return default_val;
    return it->second;
}

void Database::setMeta(const std::string& key, const std::string& value) {
    execSync("INSERT INTO metadata (key, value) VALUES (?, ?) "
             "ON CONFLICT(key) DO UPDATE SET value = excluded.value",
             {key, value});
}

// ---------------------------------------------------------------------------
// TxScope — direct (non-queued) DB operations used inside transactionSync
// ---------------------------------------------------------------------------

bool Database::TxScope::execDirect(const std::string& sql,
                                    const Params& params) {
    std::string err = execOrQuery(db, sql, params, nullptr);
    return err.empty();
}

Rows Database::TxScope::queryDirect(const std::string& sql,
                                     const Params& params) {
    Rows rows;
    execOrQuery(db, sql, params, &rows);
    return rows;
}

// ---------------------------------------------------------------------------
// transactionSync
// ---------------------------------------------------------------------------

bool Database::transactionSync(std::function<bool(TxScope&)> fn) {
    return impl_->postSync([this, fn = std::move(fn)]() -> bool {
        // BEGIN
        if (execOrQuery(impl_->db, "BEGIN", {}, nullptr) != "") return false;

        TxScope scope;
        scope.db = impl_->db;

        bool ok = false;
        try {
            ok = fn(scope);
        } catch (...) {
            ok = false;
        }

        if (ok) {
            std::string err = execOrQuery(impl_->db, "COMMIT", {}, nullptr);
            if (!err.empty()) {
                execOrQuery(impl_->db, "ROLLBACK", {}, nullptr);
                return false;
            }
            return true;
        } else {
            execOrQuery(impl_->db, "ROLLBACK", {}, nullptr);
            return false;
        }
    });
}

} // namespace anychat::db
