#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteStatementTests)

struct StatementTestFixture
{
    StatementTestFixture()
    {
        std::error_code ec;
        std::filesystem::remove("./test_data/statement_test.db", ec);

        SqliteDatabase db;
        DatabaseConfig config;
        config["database"] = "./test_data/statement_test.db";
        db.initialize(config);
        conn = db.connection();

        conn->execute(
            "CREATE TABLE statement_test ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT, "
            "age INTEGER, "
            "salary REAL, "
            "data BLOB, "
            "created DATETIME)"
        );

        db.shutdown();

        // Переоткрываем для тестов
        db.initialize(config);
        conn = db.connection();
    }

    ~StatementTestFixture()
    {
        conn.reset();
        std::error_code ec;
        std::filesystem::remove("./test_data/statement_test.db", ec);
    }

    void clearTable()
    {
        conn->execute("DELETE FROM statement_test");
        conn->execute("DELETE FROM sqlite_sequence WHERE name='statement_test'");
    }

    std::shared_ptr<IConnection> conn;
};

BOOST_FIXTURE_TEST_CASE(test_bind_and_execute_insert, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age, salary) VALUES (:name, :age, :salary)"
    );

    stmt->bindString("name", "John Doe");
    stmt->bindInt64("age", 30);
    stmt->bindDouble("salary", 50000.50);

    int64_t rows = stmt->execute();
    BOOST_CHECK_EQUAL(rows, 1);

    auto queryStmt = conn->prepareStatement(
        "SELECT name, age, salary FROM statement_test WHERE name = :name"
    );
    queryStmt->bindString("name", "John Doe");
    auto rs = queryStmt->executeQuery();
    rs->next();

    BOOST_CHECK_EQUAL(rs->valueString(0), "John Doe");
    BOOST_CHECK_EQUAL(rs->valueInt64(1), 30);
    BOOST_CHECK_CLOSE(rs->valueDouble(2), 50000.50, 0.001);
}

BOOST_FIXTURE_TEST_CASE(test_bind_null, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age) VALUES (:name, :age)"
    );

    stmt->bindString("name", "Null Age");
    stmt->bindNull("age");
    stmt->execute();

    auto queryStmt = conn->prepareStatement(
        "SELECT age FROM statement_test WHERE name = :name"
    );
    queryStmt->bindString("name", "Null Age");
    auto rs = queryStmt->executeQuery();
    rs->next();

    BOOST_CHECK(rs->isNull(0));
}

BOOST_FIXTURE_TEST_CASE(test_bind_blob, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, data) VALUES (:name, :data)"
    );

    Blob testData = { 0x01, 0x02, 0x03, 0xFF, 0x00 };
    stmt->bindString("name", "Blob Test");
    stmt->bindBlob("data", testData);
    stmt->execute();

    auto queryStmt = conn->prepareStatement(
        "SELECT data FROM statement_test WHERE name = :name"
    );
    queryStmt->bindString("name", "Blob Test");
    auto rs = queryStmt->executeQuery();
    rs->next();

    Blob retrieved = rs->valueBlob(0);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        testData.begin(), testData.end(),
        retrieved.begin(), retrieved.end()
    );
}

BOOST_FIXTURE_TEST_CASE(test_bind_datetime, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, created) VALUES (:name, :created)"
    );

    auto now = std::chrono::system_clock::now();
    stmt->bindString("name", "DateTime Test");
    stmt->bindDateTime("created", now);
    stmt->execute();

    auto queryStmt = conn->prepareStatement(
        "SELECT created FROM statement_test WHERE name = :name"
    );
    queryStmt->bindString("name", "DateTime Test");
    auto rs = queryStmt->executeQuery();
    rs->next();

    DateTime retrieved = rs->valueDateTime(0);
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - retrieved).count();
    BOOST_CHECK(std::abs(diff) <= 1);
}

BOOST_FIXTURE_TEST_CASE(test_reset_and_reuse_statement, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age) VALUES (:name, :age)"
    );

    stmt->bindString("name", "User1");
    stmt->bindInt64("age", 25);
    stmt->execute();

    stmt->reset();
    stmt->bindString("name", "User2");
    stmt->bindInt64("age", 30);
    stmt->execute();

    auto queryStmt = conn->prepareStatement(
        "SELECT name, age FROM statement_test WHERE name IN (:name1, :name2) ORDER BY name"
    );
    queryStmt->bindString("name1", "User1");
    queryStmt->bindString("name2", "User2");
    auto rs = queryStmt->executeQuery();

    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString(0), "User1");
    BOOST_CHECK_EQUAL(rs->valueInt64(1), 25);

    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString(0), "User2");
    BOOST_CHECK_EQUAL(rs->valueInt64(1), 30);
}

BOOST_FIXTURE_TEST_CASE(test_execute_query_with_parameters, StatementTestFixture)
{
    clearTable();

    // Вставляем тестовые данные
    for (int i = 1; i <= 5; ++i)
    {
        auto insertStmt = conn->prepareStatement(
            "INSERT INTO statement_test (name, age) VALUES (:name, :age)"
        );
        insertStmt->bindString("name", "User" + std::to_string(i));
        insertStmt->bindInt64("age", 20 + i);
        insertStmt->execute();
    }

    auto queryStmt = conn->prepareStatement(
        "SELECT name, age FROM statement_test WHERE age > :min_age ORDER BY age"
    );
    queryStmt->bindInt64("min_age", 22);

    auto rs = queryStmt->executeQuery();
    int count = 0;
    while (rs->next())
    {
        ++count;
        int age = rs->valueInt64(1);
        BOOST_CHECK(age > 22);
    }
    BOOST_CHECK_EQUAL(count, 3);

    queryStmt->reset();
    queryStmt->bindInt64("min_age", 20);
    rs = queryStmt->executeQuery();
    count = 0;
    while (rs->next())
    {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 5);
}

BOOST_FIXTURE_TEST_CASE(test_invalid_parameter_name_throws, StatementTestFixture)
{
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name) VALUES (:valid_name)"
    );

    BOOST_CHECK_THROW(
        stmt->bindString("invalid_name", "test"),
        std::runtime_error
    );
}

BOOST_FIXTURE_TEST_CASE(test_update_affected_rows, StatementTestFixture)
{
    clearTable();

    conn->execute("INSERT INTO statement_test (name, age) VALUES ('Update Test', 10)");

    auto stmt = conn->prepareStatement(
        "UPDATE statement_test SET age = :new_age WHERE name = :name"
    );
    stmt->bindInt64("new_age", 20);
    stmt->bindString("name", "Update Test");

    int64_t rows = stmt->execute();
    BOOST_CHECK_EQUAL(rows, 1);

    auto queryStmt = conn->prepareStatement(
        "SELECT age FROM statement_test WHERE name = :name"
    );
    queryStmt->bindString("name", "Update Test");
    auto rs = queryStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 20);
}

BOOST_FIXTURE_TEST_CASE(test_select_returns_no_rows, StatementTestFixture)
{
    clearTable();

    auto stmt = conn->prepareStatement(
        "SELECT * FROM statement_test WHERE age = :age"
    );
    stmt->bindInt64("age", 999);

    auto rs = stmt->executeQuery();
    BOOST_CHECK(!rs->next()); // Сразу false, нет данных
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
