#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/database_factory.h"
#include "vault-server/src/storage/sqlite/sqlite_database.h"

#include "test_fixture.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteDatabaseTests)

BOOST_AUTO_TEST_CASE(test_initialize_and_shutdown)
{
    SqliteDatabase db;
    DatabaseConfig config;
    config["database"] = getTestDbPath("test_init.db");

    BOOST_REQUIRE_NO_THROW(db.initialize(config));
    BOOST_REQUIRE_NO_THROW(db.shutdown());
}

BOOST_AUTO_TEST_CASE(test_initialize_twice_throws)
{
    SqliteDatabase db;
    DatabaseConfig config;
    config["database"] = getTestDbPath("test_init_twice.db");

    db.initialize(config);
    BOOST_CHECK_THROW(db.initialize(config), std::runtime_error);
    db.shutdown();
}

BOOST_AUTO_TEST_CASE(test_initialize_without_config_throws)
{
    SqliteDatabase db;
    DatabaseConfig config; // Пустая конфигурация, нет ключа "database"
    BOOST_CHECK_THROW(db.initialize(config), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_connection_returns_valid_connection)
{
    SqliteDatabase db;
    DatabaseConfig config;
    config["database"] = getTestDbPath("test_connection.db");
    db.initialize(config);

    auto conn = db.connection();
    BOOST_REQUIRE(conn != nullptr);

    // Проверяем, что соединение работает
    BOOST_CHECK_NO_THROW(conn->execute("CREATE TABLE test (id INTEGER)"));

    db.shutdown();
}

BOOST_AUTO_TEST_CASE(test_multiple_connections_not_allowed)
{
    SqliteDatabase db;
    DatabaseConfig config;
    config["database"] = getTestDbPath("test_multi_conn.db");
    db.initialize(config);

    auto conn1 = db.connection();
    BOOST_CHECK_THROW(db.connection(), std::runtime_error); // Второе соединение запрещено

    conn1.reset(); // Освобождаем первое соединение
    auto conn2 = db.connection(); // Теперь можно
    BOOST_CHECK(conn2 != nullptr);

    db.shutdown();
}

BOOST_AUTO_TEST_CASE(test_connection_before_initialize_throws)
{
    SqliteDatabase db;
    // Не вызывали initialize()
    BOOST_CHECK_THROW(db.connection(), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(test_factory_creates_sqlite)
{
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);
    BOOST_REQUIRE(db != nullptr);

    DatabaseConfig config;
    config["database"] = getTestDbPath("test_factory.db");
    db->initialize(config);

    auto conn = db->connection();
    conn->execute("CREATE TABLE factory_test (id INTEGER)");
    conn->execute("INSERT INTO factory_test VALUES (42)");

    auto stmt = conn->prepareStatement("SELECT id FROM factory_test");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 42);

    db->shutdown();
}

BOOST_AUTO_TEST_CASE(test_factory_from_connection_string)
{
    const std::string dbPath = "./test_data/test_conn_string.db";
    const std::string connStr = "sqlite://" + dbPath.substr(2);

    std::error_code ec;
    std::filesystem::remove(dbPath, ec);

    auto db = DatabaseFactory::createFromConnectionString(connStr);
    BOOST_REQUIRE(db != nullptr);

    auto conn = db->connection();
    conn->execute("CREATE TABLE conn_test (data TEXT)");
    conn->execute("INSERT INTO conn_test VALUES ('test')");

    auto stmt = conn->prepareStatement("SELECT data FROM conn_test");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString(0), "test");

    db->shutdown();
    std::filesystem::remove(dbPath, ec);
}

BOOST_AUTO_TEST_CASE(test_factory_from_invalid_connection_string_throws)
{
    std::string invalidConnStr = "invalid://path/to/db";
    BOOST_CHECK_THROW(
        DatabaseFactory::createFromConnectionString(invalidConnStr),
        std::runtime_error
    );
}

BOOST_AUTO_TEST_CASE(test_factory_unsupported_type_throws)
{
    BOOST_CHECK_THROW(
        DatabaseFactory::create(static_cast<DatabaseType>(999)),
        std::runtime_error
    );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
