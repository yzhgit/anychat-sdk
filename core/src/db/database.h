#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// Forward-declare sqlite3 for the TxScope raw handle.
struct sqlite3;

namespace anychat::db {

using DbValue = std::variant<std::nullptr_t, int64_t, double, std::string>;
using Params  = std::vector<DbValue>;
using Row     = std::unordered_map<std::string, std::string>;
using Rows    = std::vector<Row>;

using ExecCallback  = std::function<void(bool ok, std::string err)>;
using QueryCallback = std::function<void(Rows rows, std::string err)>;

class Database {
public:
    explicit Database(std::string path);
    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    // Opens the SQLite file, enables WAL mode, runs schema migrations.
    // Returns false on error (check stderr/log for details).
    bool open();

    // Drains the task queue and closes the SQLite file.
    void close();

    // -------------------------------------------------------------------------
    // Async variants — callback is invoked on the DB worker thread.
    // -------------------------------------------------------------------------
    void exec(std::string sql, Params params = {}, ExecCallback cb = nullptr);
    void query(std::string sql, Params params, QueryCallback cb);

    // -------------------------------------------------------------------------
    // Sync variants — block the calling thread until the DB thread completes.
    // -------------------------------------------------------------------------
    bool execSync(const std::string& sql, Params params = {});
    Rows querySync(const std::string& sql, Params params = {});

    // -------------------------------------------------------------------------
    // Convenience key/value metadata store (backed by the `metadata` table).
    // -------------------------------------------------------------------------
    std::string getMeta(const std::string& key,
                        const std::string& default_val = "");
    void        setMeta(const std::string& key, const std::string& value);

    // -------------------------------------------------------------------------
    // Atomic transaction helper.
    //
    // fn receives a TxScope whose exec/query methods run *directly* on the DB
    // thread (no re-queuing) to avoid deadlock.  Return false from fn to roll
    // back.  Any exception thrown inside fn also causes a rollback.
    // -------------------------------------------------------------------------
    struct TxScope {
        // Execute a statement and return true on success.
        bool execDirect(const std::string& sql, const Params& params = {});

        // Execute a query and return all result rows.
        Rows queryDirect(const std::string& sql, const Params& params = {});

        // Raw SQLite handle — for internal use by execDirect/queryDirect.
        struct sqlite3* db = nullptr;
    };

    // Returns false if fn returned false, threw an exception, or the BEGIN /
    // COMMIT themselves failed.
    bool transactionSync(std::function<bool(TxScope&)> fn);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace anychat::db
