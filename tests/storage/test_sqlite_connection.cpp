#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_connection.h"
#include "vault-server/src/storage/sqlite/sqlite_database.h"

#include "test_fixture.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteConnectionTests)

BOOST_AUTO_TEST_CASE(test_connection_create_and_use)
{
    std::string dbPath = getTestDbPath("conn_test.db");

    // Удаляем старый файл
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);

    // Проверяем, что соединение работает
    BOOST_CHECK_NO_THROW(conn.execute("CREATE TABLE test (id INTEGER)"));
    BOOST_CHECK_NO_THROW(conn.execute("INSERT INTO test VALUES (1)"));

    auto stmt = conn.prepareStatement("SELECT id FROM test");
    auto rs = stmt->executeQuery();
    BOOST_CHECK(rs->next());
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1);
}

BOOST_AUTO_TEST_CASE(test_prepare_statement)
{
    std::string dbPath = getTestDbPath("prep_test.db");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);
    conn.execute("CREATE TABLE prep_test (id INTEGER, name TEXT)");

    auto stmt = conn.prepareStatement(
        "INSERT INTO prep_test (id, name) VALUES (:id, :name)"
    );

    BOOST_REQUIRE(stmt != nullptr);

    stmt->bindInt64("id", 100);
    stmt->bindString("name", "Prepared");
    int64_t rows = stmt->execute();
    BOOST_CHECK_EQUAL(rows, 1);

    // Проверяем результат
    auto queryStmt = conn.prepareStatement("SELECT name FROM prep_test WHERE id = :id");
    queryStmt->bindInt64("id", 100);
    auto rs = queryStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString(0), "Prepared");
}

BOOST_AUTO_TEST_CASE(test_last_insert_id)
{
    std::string dbPath = getTestDbPath("last_id_test.db");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);
    conn.execute("CREATE TABLE auto_id (id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT)");

    conn.execute("INSERT INTO auto_id (data) VALUES ('test1')");
    int64_t lastId = conn.lastInsertId();
    BOOST_CHECK_EQUAL(lastId, 1);

    conn.execute("INSERT INTO auto_id (data) VALUES ('test2')");
    lastId = conn.lastInsertId();
    BOOST_CHECK_EQUAL(lastId, 2);
}

BOOST_AUTO_TEST_CASE(test_escape_string)
{
    std::string dbPath = getTestDbPath("escape_test.db");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);

    std::string unsafe = "It's a 'quoted' string";
    std::string escaped = conn.escapeString(unsafe);

    conn.execute("CREATE TABLE escape_test (text TEXT)");
    std::string sql = "INSERT INTO escape_test VALUES ('" + escaped + "')";
    conn.execute(sql);

    auto stmt = conn.prepareStatement("SELECT text FROM escape_test");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString(0), unsafe);
}

BOOST_AUTO_TEST_CASE(test_execute_returns_affected_rows)
{
    std::string dbPath = getTestDbPath("affected_test.db");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);
    conn.execute("CREATE TABLE test (id INTEGER, value INTEGER)");
    conn.execute("INSERT INTO test VALUES (1, 100)");
    conn.execute("INSERT INTO test VALUES (2, 200)");

    int64_t affected = conn.execute("UPDATE test SET value = 50 WHERE id = 1");
    BOOST_CHECK_EQUAL(affected, 1);

    affected = conn.execute("DELETE FROM test WHERE value > 150");
    BOOST_CHECK_EQUAL(affected, 1);
}

BOOST_AUTO_TEST_CASE(test_execute_invalid_sql_throws)
{
    std::string dbPath = getTestDbPath("invalid_test.db");
    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    SqliteConnection conn(dbPath);

    BOOST_CHECK_THROW(
        conn.execute("SELECT * FROM non_existent_table"),
        std::runtime_error
    );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
