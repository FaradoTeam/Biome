#include <boost/test/unit_test.hpp>
#include "test_fixture.h"
#include "vault-server/src/storage/sqlite/sqlite_database.h"

using namespace db::sqlite;

namespace db::test
{

// Тестовый набор для SqliteStatement
BOOST_AUTO_TEST_SUITE(SqliteStatementTests)

/**
 * @brief Тест: привязка параметров и выполнение INSERT
 * 
 * Проверяет создание подготовленного запроса для вставки,
 * привязку параметров различных типов и выполнение.
 */
BOOST_FIXTURE_TEST_CASE(test_bind_and_execute_insert, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age, salary) VALUES (@name, @age, @salary)"
    );

    // Привязываем параметры разных типов
    stmt->bindString("@name", "John Doe");
    stmt->bindInt64("@age", 30);
    stmt->bindDouble("@salary", 50000.50);

    int64_t rows = stmt->execute();
    BOOST_CHECK_EQUAL(rows, 1);  // Вставлена одна строка

    // Проверяем, что данные сохранились корректно
    auto rs = db.query("SELECT name, age, salary FROM statement_test WHERE name = 'John Doe'");
    rs->next();
    BOOST_CHECK_EQUAL(rs->getString(0), "John Doe");
    BOOST_CHECK_EQUAL(rs->getInt64(1), 30);
    BOOST_CHECK_CLOSE(rs->getDouble(2), 50000.50, 0.001);
}

/**
 * @brief Тест: привязка NULL значения
 * 
 * Проверяет, что bindNull() корректно устанавливает NULL в БД.
 */
BOOST_FIXTURE_TEST_CASE(test_bind_null, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age) VALUES (@name, @age)"
    );

    stmt->bindString("@name", "Null Age");
    stmt->bindNull("@age");  // Устанавливаем NULL
    stmt->execute();

    // Проверяем, что в БД действительно NULL
    auto rs = db.query("SELECT age FROM statement_test WHERE name = 'Null Age'");
    rs->next();
    BOOST_CHECK(rs->isNull(0));
}

/**
 * @brief Тест: привязка BLOB данных
 * 
 * Проверяет вставку и чтение бинарных данных.
 */
BOOST_FIXTURE_TEST_CASE(test_bind_blob, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, data) VALUES (@name, @data)"
    );

    // Подготавливаем бинарные данные
    Blob testData = { 0x01, 0x02, 0x03, 0xFF, 0x00 };
    stmt->bindString("@name", "Blob Test");
    stmt->bindBlob("@data", testData);
    stmt->execute();

    // Читаем и проверяем
    auto rs = db.query("SELECT data FROM statement_test WHERE name = 'Blob Test'");
    rs->next();
    Blob retrieved = rs->getBlob(0);

    BOOST_CHECK_EQUAL_COLLECTIONS(testData.begin(), testData.end(), retrieved.begin(), retrieved.end());
}

/**
 * @brief Тест: привязка даты и времени
 * 
 * Проверяет работу с типом DATETIME.
 */
BOOST_FIXTURE_TEST_CASE(test_bind_datetime, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, created) VALUES (@name, @created)"
    );

    auto now = std::chrono::system_clock::now();
    stmt->bindString("@name", "DateTime Test");
    stmt->bindDateTime("@created", now);
    stmt->execute();

    // Читаем и проверяем (допускаем погрешность в 1 секунду)
    auto rs = db.query("SELECT created FROM statement_test WHERE name = 'DateTime Test'");
    rs->next();
    DateTime retrieved = rs->getDateTime(0);

    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - retrieved).count();
    BOOST_CHECK(std::abs(diff) <= 1);
}

/**
 * @brief Тест: многократное использование и сброс запроса
 * 
 * Проверяет, что reset() позволяет повторно использовать
 * подготовленный запрос с разными параметрами.
 */
BOOST_FIXTURE_TEST_CASE(test_multiple_bind_and_reset, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name, age) VALUES (@name, @age)"
    );

    // Первая вставка
    stmt->bindString("@name", "User1");
    stmt->bindInt64("@age", 25);
    stmt->execute();

    // Сбрасываем и делаем вторую вставку
    stmt->reset();
    stmt->bindString("@name", "User2");
    stmt->bindInt64("@age", 30);
    stmt->execute();

    // Проверяем обе вставки
    auto rs = db.query("SELECT name, age FROM statement_test WHERE name IN ('User1', 'User2') ORDER BY name");

    rs->next();
    BOOST_CHECK_EQUAL(rs->getString(0), "User1");
    BOOST_CHECK_EQUAL(rs->getInt64(1), 25);

    rs->next();
    BOOST_CHECK_EQUAL(rs->getString(0), "User2");
    BOOST_CHECK_EQUAL(rs->getInt64(1), 30);
}

/**
 * @brief Тест: выполнение SELECT через подготовленный запрос
 * 
 * Проверяет executeQuery() для запросов, возвращающих данные.
 */
BOOST_FIXTURE_TEST_CASE(test_execute_query_with_statement, StatementTestFixture)
{
    // Полная очистка и пересоздание таблицы для чистоты теста
    db.execute("DROP TABLE IF EXISTS statement_test");
    db.execute(
        "CREATE TABLE IF NOT EXISTS statement_test ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT, "
        "age INTEGER, "
        "salary REAL, "
        "data BLOB, "
        "created DATETIME)"
    );

    auto conn = db.getConnection();

    // Вставляем тестовые данные
    for (int i = 1; i <= 5; ++i)
    {
        auto insertStmt = conn->prepareStatement(
            "INSERT INTO statement_test (name, age) VALUES (@name, @age)"
        );
        std::string name = "User" + std::to_string(i);
        int age = 20 + i;  // ages: 21, 22, 23, 24, 25
        insertStmt->bindString("@name", name);
        insertStmt->bindInt64("@age", age);
        int64_t rows = insertStmt->execute();
        BOOST_CHECK_EQUAL(rows, 1);
    }

    // Создаём подготовленный запрос для SELECT с фильтром
    auto queryStmt = conn->prepareStatement(
        "SELECT name, age FROM statement_test WHERE age > @min_age ORDER BY age"
    );

    // Фильтр: возраст > 22 (должны получить 3 записи: ages 23, 24, 25)
    queryStmt->bindInt64("@min_age", 22);

    auto rs = queryStmt->executeQuery();
    int count = 0;
    while (rs->next())
    {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 3);

    // Сбрасываем и пробуем другой фильтр
    queryStmt->reset();
    queryStmt->bindInt64("@min_age", 20);
    rs = queryStmt->executeQuery();
    count = 0;
    while (rs->next())
    {
        ++count;
    }
    BOOST_CHECK_EQUAL(count, 5);
}

/**
 * @brief Тест: невалидное имя параметра
 * 
 * Проверяет, что привязка к несуществующему параметру
 * вызывает исключение.
 */
BOOST_FIXTURE_TEST_CASE(test_invalid_parameter_name, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO statement_test (name) VALUES (@valid_name)"
    );

    // Пытаемся привязать значение к несуществующему параметру
    BOOST_CHECK_THROW(stmt->bindString("@invalid_name", "test"), std::runtime_error);
}

/**
 * @brief Тест: выполнение UPDATE через подготовленный запрос
 * 
 * Проверяет, что execute() возвращает количество затронутых строк
 * для операций UPDATE/DELETE.
 */
BOOST_FIXTURE_TEST_CASE(test_execute_non_insert_statement, StatementTestFixture)
{
    clearTable();

    auto conn = db.getConnection();

    // Вставляем тестовую запись для обновления
    conn->execute("INSERT INTO statement_test (name, age) VALUES ('Update Test', 10)");

    // Создаём запрос на обновление
    auto stmt = conn->prepareStatement(
        "UPDATE statement_test SET age = @new_age WHERE name = @name"
    );
    stmt->bindInt64("@new_age", 20);
    stmt->bindString("@name", "Update Test");

    int64_t rows = stmt->execute();
    BOOST_CHECK_EQUAL(rows, 1);  // Обновлена одна строка

    // Проверяем результат обновления
    auto rs = db.query("SELECT age FROM statement_test WHERE name = 'Update Test'");
    rs->next();
    BOOST_CHECK_EQUAL(rs->getInt64(0), 20);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
