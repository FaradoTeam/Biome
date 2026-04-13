#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteResultSetTests)

struct ResultSetTestFixture
{
    ResultSetTestFixture()
    {
        std::error_code ec;
        std::filesystem::remove("./test_data/resultset_test.db", ec);

        SqliteDatabase db;
        DatabaseConfig config;
        config["database"] = "./test_data/resultset_test.db";
        db.initialize(config);
        conn = db.connection();

        conn->execute(
            "CREATE TABLE resultset_test ("
            "id INTEGER PRIMARY KEY, "
            "int_col INTEGER, "
            "float_col REAL, "
            "text_col TEXT, "
            "null_col INTEGER, "
            "blob_col BLOB)"
        );

        conn->execute(
            "INSERT INTO resultset_test (id, int_col, float_col, text_col, null_col, blob_col) "
            "VALUES (1, 42, 3.14159, 'Hello World', NULL, X'010203FF')"
        );

        conn->execute(
            "INSERT INTO resultset_test (id, int_col, float_col, text_col, null_col, blob_col) "
            "VALUES (2, 100, 2.71828, 'Second Row', NULL, NULL)"
        );

        db.shutdown();

        // Переоткрываем для тестов
        db.initialize(config);
        conn = db.connection();
    }

    ~ResultSetTestFixture()
    {
        conn.reset();
        std::error_code ec;
        std::filesystem::remove("./test_data/resultset_test.db", ec);
    }

    std::shared_ptr<IConnection> conn;
};

BOOST_FIXTURE_TEST_CASE(test_next_and_row_iteration, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id FROM resultset_test ORDER BY id");
    auto rs = stmt->executeQuery();

    BOOST_CHECK(rs->next());
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1);

    BOOST_CHECK(rs->next());
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 2);

    BOOST_CHECK(!rs->next());
}

BOOST_FIXTURE_TEST_CASE(test_column_count, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT * FROM resultset_test");
    auto rs = stmt->executeQuery();
    BOOST_CHECK_EQUAL(rs->columnCount(), 6);
}

BOOST_FIXTURE_TEST_CASE(test_column_name_and_index, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id, int_col, text_col FROM resultset_test");
    auto rs = stmt->executeQuery();

    BOOST_CHECK_EQUAL(rs->columnName(0), "id");
    BOOST_CHECK_EQUAL(rs->columnName(1), "int_col");
    BOOST_CHECK_EQUAL(rs->columnName(2), "text_col");

    BOOST_CHECK_EQUAL(rs->columnIndex("id"), 0);
    BOOST_CHECK_EQUAL(rs->columnIndex("int_col"), 1);
    BOOST_CHECK_EQUAL(rs->columnIndex("text_col"), 2);
}

BOOST_FIXTURE_TEST_CASE(test_value_by_index, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT int_col, float_col, text_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    FieldValue intVal = rs->value(0);
    BOOST_CHECK_EQUAL(intVal.asInt64(), 42);

    FieldValue floatVal = rs->value(1);
    BOOST_CHECK_CLOSE(floatVal.asDouble(), 3.14159, 0.00001);

    FieldValue textVal = rs->value(2);
    BOOST_CHECK_EQUAL(textVal.asString(), "Hello World");
}

BOOST_FIXTURE_TEST_CASE(test_value_by_name, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT int_col, float_col, text_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    FieldValue intVal = rs->value("int_col");
    BOOST_CHECK_EQUAL(intVal.asInt64(), 42);

    FieldValue floatVal = rs->value("float_col");
    BOOST_CHECK_CLOSE(floatVal.asDouble(), 3.14159, 0.00001);

    FieldValue textVal = rs->value("text_col");
    BOOST_CHECK_EQUAL(textVal.asString(), "Hello World");
}

BOOST_FIXTURE_TEST_CASE(test_typed_getters, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT int_col, float_col, text_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    BOOST_CHECK_EQUAL(rs->valueInt64(0), 42);
    BOOST_CHECK_EQUAL(rs->valueInt64("int_col"), 42);

    BOOST_CHECK_CLOSE(rs->valueDouble(1), 3.14159, 0.00001);
    BOOST_CHECK_CLOSE(rs->valueDouble("float_col"), 3.14159, 0.00001);

    BOOST_CHECK_EQUAL(rs->valueString(2), "Hello World");
    BOOST_CHECK_EQUAL(rs->valueString("text_col"), "Hello World");
}

BOOST_FIXTURE_TEST_CASE(test_is_null, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT null_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    BOOST_CHECK(rs->isNull(0));
    BOOST_CHECK(rs->isNull("null_col"));
}

BOOST_FIXTURE_TEST_CASE(test_blob_value, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT blob_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    Blob expected = { 0x01, 0x02, 0x03, 0xFF };

    Blob actual = rs->valueBlob(0);
    BOOST_CHECK_EQUAL_COLLECTIONS(
        expected.begin(), expected.end(),
        actual.begin(), actual.end()
    );

    Blob actualByName = rs->valueBlob("blob_col");
    BOOST_CHECK_EQUAL_COLLECTIONS(
        expected.begin(), expected.end(),
        actualByName.begin(), actualByName.end()
    );
}

BOOST_FIXTURE_TEST_CASE(test_null_blob_throws, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT blob_col FROM resultset_test WHERE id = 2"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    BOOST_CHECK(rs->isNull(0));
    BOOST_CHECK_THROW(rs->valueBlob(0), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_invalid_column_index_throws, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id FROM resultset_test");
    auto rs = stmt->executeQuery();
    rs->next();

    BOOST_CHECK_THROW(rs->columnName(100), std::out_of_range);
    BOOST_CHECK_THROW(rs->value(100), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_invalid_column_name_throws, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id FROM resultset_test");
    auto rs = stmt->executeQuery();

    BOOST_CHECK_THROW(rs->columnIndex("non_existent"), std::runtime_error);
    BOOST_CHECK_THROW(rs->value("non_existent"), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_value_before_next_throws, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id FROM resultset_test");
    auto rs = stmt->executeQuery();
    // Не вызывали rs->next()
    BOOST_CHECK_THROW(rs->value(0), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_type_mismatch_throws, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement(
        "SELECT text_col FROM resultset_test WHERE id = 1"
    );
    auto rs = stmt->executeQuery();
    rs->next();

    BOOST_CHECK_THROW(rs->valueInt64(0), std::runtime_error);
    BOOST_CHECK_THROW(rs->valueDouble(0), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(test_column_index_caching, ResultSetTestFixture)
{
    auto stmt = conn->prepareStatement("SELECT id, int_col, text_col FROM resultset_test");
    auto rs = stmt->executeQuery();

    int idx1 = rs->columnIndex("text_col");
    int idx2 = rs->columnIndex("text_col");

    BOOST_CHECK_EQUAL(idx1, idx2);
    BOOST_CHECK_EQUAL(idx1, 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
