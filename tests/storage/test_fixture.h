#pragma once

#include <filesystem>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

/**
 * @brief Фикстура для тестирования подготовленных запросов (Statement)
 * 
 * Создаёт временную БД с таблицей statement_test и автоматически очищает
 * её перед каждым тестом. Обеспечивает изоляцию тестов друг от друга.
 */
struct StatementTestFixture
{
    StatementTestFixture()
    {
        // Удаляем старый файл БД, если существует
        std::error_code ec;
        std::filesystem::remove("./test_data/statement_test.db", ec);

        // Настраиваем конфигурацию и инициализируем БД
        config["database"] = "./test_data/statement_test.db";
        db.initialize(config);

        // Создаём тестовую таблицу с различными типами данных
        db.execute(
            "CREATE TABLE IF NOT EXISTS statement_test ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "   // Автоинкрементный первичный ключ
            "name TEXT, "                              // Текстовое поле
            "age INTEGER, "                            // Целочисленное поле
            "salary REAL, "                            // Вещественное поле
            "data BLOB, "                              // Бинарное поле
            "created DATETIME)"                        // Поле даты/времени
        );
    }

    /**
     * @brief Полная очистка таблицы перед каждым тестом
     * 
     * Удаляет все записи и сбрасывает счётчик автоинкремента
     */
    void clearTable()
    {
        db.execute("DELETE FROM statement_test");
        // Сброс автоинкремента через системную таблицу sqlite_sequence
        auto rs = db.query(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='statement_test'"
        );
        if (rs->next())
        {
            db.execute("DELETE FROM sqlite_sequence WHERE name='statement_test'");
        }
    }

    // Деструктор: очищаем ресурсы и удаляем файл БД
    virtual ~StatementTestFixture()
    {
        try
        {
            db.shutdown();
            std::error_code ec;
            std::filesystem::remove("./test_data/statement_test.db", ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке
        }
    }

    db::sqlite::SqliteDatabase db;  // Экземпляр БД
    DatabaseConfig config;          // Конфигурация БД
};

/**
 * @brief Фикстура для тестирования наборов результатов (ResultSet)
 * 
 * Создаёт БД с предзаполненными тестовыми данными для проверки
 * операций чтения, навигации и преобразования типов.
 */
struct ResultSetTestFixture
{
    ResultSetTestFixture()
    {
        // Удаляем старый файл БД
        std::error_code ec;
        std::filesystem::remove("./test_data/resultset_test.db", ec);

        // Инициализируем БД
        config["database"] = "./test_data/resultset_test.db";
        db.initialize(config);

        // Создаём тестовую таблицу со всеми поддерживаемыми типами
        db.execute(
            "CREATE TABLE IF NOT EXISTS resultset_test ("
            "id INTEGER PRIMARY KEY, "
            "int_col INTEGER, "
            "float_col REAL, "
            "text_col TEXT, "
            "null_col INTEGER, "
            "blob_col BLOB)"
        );

        // Очищаем и заполняем тестовыми данными
        db.execute("DELETE FROM resultset_test");

        // Первая запись: все поля заполнены
        db.execute(
            "INSERT INTO resultset_test (id, int_col, float_col, text_col, null_col, blob_col) "
            "VALUES (1, 42, 3.14159, 'Hello World', NULL, X'010203FF')"
        );
        
        // Вторая запись: NULL значения для проверки isNull()
        db.execute(
            "INSERT INTO resultset_test (id, int_col, float_col, text_col, null_col, blob_col) "
            "VALUES (2, 100, 2.71828, 'Second Row', NULL, NULL)"
        );
    }

    virtual ~ResultSetTestFixture()
    {
        try
        {
            db.shutdown();
            std::error_code ec;
            std::filesystem::remove("./test_data/resultset_test.db", ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке
        }
    }

    db::sqlite::SqliteDatabase db;
    DatabaseConfig config;
};

/**
 * @brief Фикстура для тестирования транзакций
 * 
 * Создаёт БД с таблицей банковских счетов для проверки
 * атомарности операций и отката изменений.
 */
struct TransactionTestFixture
{
    TransactionTestFixture()
    {
        std::error_code ec;
        std::filesystem::remove("./test_data/transaction_test.db", ec);

        config["database"] = "./test_data/transaction_test.db";
        db.initialize(config);

        // Создаём таблицу счетов с балансом
        db.execute("CREATE TABLE IF NOT EXISTS tx_accounts ("
                   "id INTEGER PRIMARY KEY, "
                   "name TEXT, "
                   "balance INTEGER)");

        // Очищаем и заполняем начальными данными
        db.execute("DELETE FROM tx_accounts");
        db.execute("INSERT INTO tx_accounts VALUES (1, 'Alice', 1000)");
        db.execute("INSERT INTO tx_accounts VALUES (2, 'Bob', 500)");
    }

    virtual ~TransactionTestFixture()
    {
        try
        {
            db.shutdown();
            std::error_code ec;
            std::filesystem::remove("./test_data/transaction_test.db", ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке
        }
    }

    db::sqlite::SqliteDatabase db;
    DatabaseConfig config;
};

/**
 * @brief Фикстура для тестирования соединений с БД
 * 
 * Создаёт пустую БД для проверки операций соединения:
 * открытие, закрытие, выполнение запросов.
 */
struct ConnectionTestFixture
{
    ConnectionTestFixture()
    {
        std::error_code ec;
        std::filesystem::remove("./test_data/connection_test.db", ec);

        config["database"] = "./test_data/connection_test.db";
        db.initialize(config);
    }

    virtual ~ConnectionTestFixture()
    {
        try
        {
            db.shutdown();
            std::error_code ec;
            std::filesystem::remove("./test_data/connection_test.db", ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке
        }
    }

    db::sqlite::SqliteDatabase db;
    DatabaseConfig config;
};

} // namespace db::test
