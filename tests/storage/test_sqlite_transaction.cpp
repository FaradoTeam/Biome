#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteTransactionTests)

struct TransactionTestFixture
{
    TransactionTestFixture()
    {
        std::error_code ec;
        std::filesystem::remove("./test_data/transaction_test.db", ec);

        SqliteDatabase db;
        DatabaseConfig config;
        config["database"] = "./test_data/transaction_test.db";
        db.initialize(config);
        conn = db.connection();

        conn->execute(
            "CREATE TABLE tx_accounts ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT, "
            "balance INTEGER)"
        );

        conn->execute("INSERT INTO tx_accounts VALUES (1, 'Alice', 1000)");
        conn->execute("INSERT INTO tx_accounts VALUES (2, 'Bob', 500)");

        db.shutdown();

        db.initialize(config);
        conn = db.connection();
    }

    ~TransactionTestFixture()
    {
        conn.reset();
        std::error_code ec;
        std::filesystem::remove("./test_data/transaction_test.db", ec);
    }

    std::shared_ptr<IConnection> conn;
};

BOOST_FIXTURE_TEST_CASE(test_commit_changes, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    BOOST_CHECK(tx->isActive());

    conn->execute("UPDATE tx_accounts SET balance = balance - 200 WHERE id = 1");
    conn->execute("UPDATE tx_accounts SET balance = balance + 200 WHERE id = 2");

    tx->commit();
    BOOST_CHECK(!tx->isActive());

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = :id");

    stmt->bindInt64("id", 1);
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 800);

    stmt->reset();
    stmt->bindInt64("id", 2);
    rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 700);
}

BOOST_FIXTURE_TEST_CASE(test_rollback_changes, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();

    conn->execute("UPDATE tx_accounts SET balance = 0 WHERE id = 1");
    conn->execute("UPDATE tx_accounts SET balance = 0 WHERE id = 2");

    tx->rollback();
    BOOST_CHECK(!tx->isActive());

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = :id");

    stmt->bindInt64("id", 1);
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1000);

    stmt->reset();
    stmt->bindInt64("id", 2);
    rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 500);
}

BOOST_FIXTURE_TEST_CASE(test_auto_rollback_on_destruction, TransactionTestFixture)
{
    {
        auto tx = conn->beginTransaction();
        conn->execute("UPDATE tx_accounts SET balance = 999 WHERE id = 1");
        // Транзакция не коммитится - при выходе из блока произойдёт откат
    }

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1000);
}

BOOST_FIXTURE_TEST_CASE(test_commit_twice_throws, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->commit();
    BOOST_CHECK_THROW(tx->commit(), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_rollback_twice_throws, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->rollback();
    BOOST_CHECK_THROW(tx->rollback(), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_operations_after_commit_work, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->commit();

    BOOST_CHECK_NO_THROW(conn->execute("UPDATE tx_accounts SET balance = 500 WHERE id = 1"));

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 500);
}

BOOST_FIXTURE_TEST_CASE(test_nested_transactions_not_supported, TransactionTestFixture)
{
    auto tx1 = conn->beginTransaction();
    BOOST_CHECK_THROW(conn->beginTransaction(), std::runtime_error);
    tx1->rollback();
}

BOOST_FIXTURE_TEST_CASE(test_transaction_guard_raii, TransactionTestFixture)
{
    struct TransactionGuard
    {
        TransactionGuard(std::unique_ptr<ITransaction> tx)
            : m_tx(std::move(tx))
        {
        }

        ~TransactionGuard()
        {
            if (m_tx && m_tx->isActive())
                m_tx->rollback();
        }

        void commit()
        {
            if (m_tx)
                m_tx->commit();
        }

        ITransaction* operator->() { return m_tx.get(); }

    private:
        std::unique_ptr<ITransaction> m_tx;
    };

    {
        TransactionGuard guard(conn->beginTransaction());
        conn->execute("UPDATE tx_accounts SET balance = 3000 WHERE id = 1");
        guard.commit();
    }

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 3000);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
