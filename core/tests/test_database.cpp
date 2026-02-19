#include <gtest/gtest.h>
#include "db/database.h"
// Note: runMigrations() takes a raw sqlite3*, which is only accessible internally.
// Database::open() already calls runMigrations() internally.
// We test the full Database API via the public interface.

using namespace anychat::db;
using TxScope = Database::TxScope;

// ---------------------------------------------------------------------------
// Fixture: opens a fresh in-memory database for each test.
// ---------------------------------------------------------------------------
class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<Database>(":memory:");
        ASSERT_TRUE(db_->open()) << "Failed to open in-memory database";
    }

    void TearDown() override {
        db_->close();
        db_.reset();
    }

    std::unique_ptr<Database> db_;
};

// ---------------------------------------------------------------------------
// 1. OpenAndMigrate
//    Verify that opening an in-memory DB (which calls runMigrations internally)
//    succeeds without errors.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, OpenAndMigrate) {
    // SetUp already called open() and verified it returned true.
    // Query the schema version as additional confirmation.
    Rows rows = db_->querySync("PRAGMA user_version");
    ASSERT_FALSE(rows.empty());
    EXPECT_EQ(rows[0].at("user_version"), "1");
}

// ---------------------------------------------------------------------------
// 2. ExecAndQuerySync
//    Insert a row into `metadata` via execSync(), then read it back with
//    querySync(). Verify the key and value round-trip correctly.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, ExecAndQuerySync) {
    bool ok = db_->execSync(
        "INSERT INTO metadata (key, value) VALUES (?, ?)",
        {std::string("testkey"), std::string("testvalue")});
    ASSERT_TRUE(ok) << "execSync INSERT failed";

    Rows rows = db_->querySync(
        "SELECT key, value FROM metadata WHERE key = ?",
        {std::string("testkey")});

    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].at("key"),   "testkey");
    EXPECT_EQ(rows[0].at("value"), "testvalue");
}

// ---------------------------------------------------------------------------
// 3. TransactionCommit
//    Insert two rows inside a transaction that returns true. Both rows should
//    be present after the transaction completes.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, TransactionCommit) {
    bool committed = db_->transactionSync([](TxScope& tx) -> bool {
        bool ok1 = tx.execDirect(
            "INSERT INTO metadata (key, value) VALUES (?, ?)",
            {std::string("tx_key1"), std::string("val1")});
        bool ok2 = tx.execDirect(
            "INSERT INTO metadata (key, value) VALUES (?, ?)",
            {std::string("tx_key2"), std::string("val2")});
        return ok1 && ok2;
    });
    ASSERT_TRUE(committed) << "Transaction should have committed";

    Rows rows = db_->querySync(
        "SELECT key FROM metadata WHERE key IN ('tx_key1','tx_key2') ORDER BY key");
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].at("key"), "tx_key1");
    EXPECT_EQ(rows[1].at("key"), "tx_key2");
}

// ---------------------------------------------------------------------------
// 4. TransactionRollback
//    A transaction that throws inside fn should roll back; no rows committed.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, TransactionRollback) {
    bool committed = db_->transactionSync([](TxScope& tx) -> bool {
        tx.execDirect(
            "INSERT INTO metadata (key, value) VALUES (?, ?)",
            {std::string("rollback_key"), std::string("should_not_persist")});
        // Signal rollback by returning false.
        return false;
    });
    EXPECT_FALSE(committed) << "Transaction should have rolled back";

    Rows rows = db_->querySync(
        "SELECT key FROM metadata WHERE key = ?",
        {std::string("rollback_key")});
    EXPECT_TRUE(rows.empty()) << "Rolled-back row should not be present";
}

// ---------------------------------------------------------------------------
// 4b. TransactionRollbackOnException
//    A transaction that throws should also roll back.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, TransactionRollbackOnException) {
    bool committed = db_->transactionSync([](TxScope& tx) -> bool {
        tx.execDirect(
            "INSERT INTO metadata (key, value) VALUES (?, ?)",
            {std::string("exception_key"), std::string("nope")});
        throw std::runtime_error("intentional error");
        return true; // unreachable
    });
    EXPECT_FALSE(committed) << "Transaction should have rolled back on exception";

    Rows rows = db_->querySync(
        "SELECT key FROM metadata WHERE key = ?",
        {std::string("exception_key")});
    EXPECT_TRUE(rows.empty()) << "Row inserted before throw should not persist";
}

// ---------------------------------------------------------------------------
// 5. GetSetMeta
//    setMeta() + getMeta() round-trips correctly.
//    getMeta() returns the default value for unknown keys.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, GetSetMeta) {
    db_->setMeta("foo", "bar");
    std::string val = db_->getMeta("foo", "default");
    EXPECT_EQ(val, "bar");

    // Missing key should return the supplied default.
    std::string missing = db_->getMeta("does_not_exist", "default");
    EXPECT_EQ(missing, "default");
}

// ---------------------------------------------------------------------------
// 5b. GetMetaOverwrite
//    Calling setMeta twice on the same key should update the value.
// ---------------------------------------------------------------------------
TEST_F(DatabaseTest, GetMetaOverwrite) {
    db_->setMeta("key", "first");
    db_->setMeta("key", "second");
    EXPECT_EQ(db_->getMeta("key", ""), "second");
}
