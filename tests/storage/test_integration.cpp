#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/database_factory.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(IntegrationTests)

/**
 * @brief Полный рабочий процесс: создание схемы, вставка, выборка, обновление, удаление
 *
 * Проверяет базовый сценарий работы с реляционной БД через абстрактный интерфейс.
 * Включает работу с подготовленными запросами, транзакциями и проверку целостности данных.
 */
BOOST_AUTO_TEST_CASE(test_complete_workflow)
{
    // Создаём экземпляр БД через фабрику (тип SQLite)
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);

    // Настраиваем и инициализируем
    DatabaseConfig config;
    config["database"] = "./test_data/integration_test.db";
    db->initialize(config);

    auto conn = db->connection();

    // 1. Создание схемы (таблицы продуктов)
    conn->execute("DROP TABLE IF EXISTS products");
    conn->execute(R"(
        CREATE TABLE products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            price REAL,
            stock INTEGER,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )");

    // 2. Вставка данных через подготовленные запросы
    // Подготовленный запрос позволяет безопасно вставлять данные (защита от SQL-инъекций)
    auto insertStmt = conn->prepareStatement(
        "INSERT INTO products (name, price, stock) VALUES (:name, :price, :stock)"
    );

    // Тестовые данные: названия товаров, цены и остатки на складе
    std::vector<std::tuple<std::string, double, int>> products = {
        { "Laptop", 999.99, 10 },
        { "Mouse", 25.50, 50 },
        { "Keyboard", 75.00, 30 }
    };

    for (const auto& [name, price, stock] : products)
    {
        insertStmt->reset(); // Сбрасываем состояние перед новой вставкой
        insertStmt->bindString("name", name); // Привязываем параметр :name
        insertStmt->bindDouble("price", price); // Привязываем параметр :price
        insertStmt->bindInt64("stock", stock); // Привязываем параметр :stock
        int64_t result = insertStmt->execute(); // Выполняем вставку
        BOOST_CHECK_EQUAL(result, 1); // Должна быть затронута 1 строка
    }

    // 3. Выборка с фильтром (цены выше 50, сортировка по цене)
    auto queryStmt = conn->prepareStatement(
        "SELECT name, price, stock FROM products WHERE price > :min_price ORDER BY price"
    );
    queryStmt->bindDouble("min_price", 50.0);

    auto rs = queryStmt->executeQuery();

    // Ожидаемые результаты (в порядке возрастания цены)
    std::vector<std::string> expectedNames = { "Keyboard", "Laptop" };
    std::vector<double> expectedPrices = { 75.00, 999.99 };

    int idx = 0;
    while (rs->next())
    {
        BOOST_CHECK_EQUAL(rs->valueString("name"), expectedNames[idx]);
        BOOST_CHECK_CLOSE(rs->valueDouble("price"), expectedPrices[idx], 0.01);
        ++idx;
    }
    BOOST_CHECK_EQUAL(idx, 2); // Должно быть ровно 2 товара

    // 4. Обновление в транзакции (списание товаров со склада)
    {
        auto tx = conn->beginTransaction(); // Начинаем транзакцию
        conn->execute("UPDATE products SET stock = stock - 1 WHERE name = 'Laptop'");
        conn->execute("UPDATE products SET stock = stock - 5 WHERE name = 'Mouse'");
        tx->commit(); // Фиксируем изменения
    }

    // Проверяем обновление остатков
    auto checkStmt = conn->prepareStatement("SELECT stock FROM products WHERE name = :name");

    checkStmt->bindString("name", "Laptop");
    rs = checkStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 9); // Было 10, стало 9

    checkStmt->reset(); // Переиспользуем подготовленный запрос
    checkStmt->bindString("name", "Mouse");
    rs = checkStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 45); // Было 50, стало 45

    // 5. Удаление товаров с нулевым остатком (в данном случае таких нет)
    auto deleteStmt = conn->prepareStatement("DELETE FROM products WHERE stock = :stock");
    deleteStmt->bindInt64("stock", 0);
    int64_t deleted = deleteStmt->execute();
    BOOST_CHECK_EQUAL(deleted, 0); // Ничего не удалено

    // Финальная проверка: в таблице всё ещё 3 товара
    auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM products");
    rs = countStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 3);

    db->shutdown();
}

/**
 * @brief Тест конкурентных операций в многопоточной среде
 *
 * Проверяет, что несколько потоков могут одновременно обновлять один и тот же счётчик
 * с использованием механизма повторных попыток при блокировках (busy/locked).
 * Включает WAL-режим SQLite для лучшей многопоточной производительности.
 */
BOOST_AUTO_TEST_CASE(test_concurrent_operations)
{
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);

    DatabaseConfig config;
    config["database"] = "./test_data/concurrent_test.db";
    db->initialize(config);

    // ОДНО соединение, разделяемое между всеми потоками (SQLite может работать так в WAL-режиме)
    auto sharedConn = db->connection();

    sharedConn->execute("DROP TABLE IF EXISTS counter");
    sharedConn->execute("CREATE TABLE counter (id INTEGER PRIMARY KEY, value INTEGER)");
    sharedConn->execute("INSERT INTO counter VALUES (1, 0)");

    // Включаем многопоточный режим SQLite: WAL (Write-Ahead Logging) + синхронизация NORMAL
    sharedConn->execute("PRAGMA journal_mode=WAL");
    sharedConn->execute("PRAGMA synchronous=NORMAL");

    std::vector<std::thread> threads;
    std::atomic<int> successes { 0 }; // Счётчик успешных обновлений
    std::mutex coutMutex; // Для синхронизированного вывода (не критично)

    // Запускаем 5 потоков, каждый пытается увеличить счётчик на 1
    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(
            [&sharedConn, &successes, &coutMutex]()
            {
                try
                {
                    int retries = 3; // Максимум 3 попытки при конфликтах
                    while (retries-- > 0)
                    {
                        try
                        {
                            // Прямое выполнение UPDATE — возвращает количество затронутых строк
                            const int64_t result = sharedConn->execute(
                                "UPDATE counter SET value = value + 1 WHERE id = 1"
                            );

                            if (result > 0)
                            {
                                ++successes; // Успешное обновление
                            }
                            break; // Выходим из цикла повторных попыток
                        }
                        catch (const std::exception& e)
                        {
                            // Если ошибка не связана с блокировкой — пробрасываем дальше
                            if (retries == 0
                                || (std::string(e.what()).find("locked") == std::string::npos
                                    && std::string(e.what()).find("busy") == std::string::npos))
                            {
                                throw;
                            }
                            // Иначе ждём с увеличивающейся задержкой и повторяем
                            std::this_thread::sleep_for(std::chrono::milliseconds(10 * (3 - retries)));
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    std::cout
                        << "Concurrent operation failed: "
                        << e.what() << std::endl;
                }
            }
        );
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads)
    {
        t.join();
    }

    // Проверяем финальное значение счётчика
    auto checkStmt = sharedConn->prepareStatement("SELECT value FROM counter WHERE id = 1");
    auto rs = checkStmt->executeQuery();
    rs->next();
    int64_t finalValue = rs->valueInt64(0);

    BOOST_CHECK_EQUAL(finalValue, 5); // Счётчик должен увеличиться ровно на 5
    BOOST_CHECK_EQUAL(successes, 5); // Все 5 операций должны быть успешными

    db->shutdown();
}

/**
 * @brief Тест обработки ошибок и восстановления после них
 *
 * Проверяет, что после возникновения ошибки (например, нарушение уникальности)
 * база данных остаётся в работоспособном состоянии и позволяет выполнять корректные операции.
 */
BOOST_AUTO_TEST_CASE(test_error_handling_and_recovery)
{
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);

    DatabaseConfig config;
    config["database"] = "./test_data/error_test.db";
    db->initialize(config);

    auto conn = db->connection();

    conn->execute("DROP TABLE IF EXISTS error_test");
    conn->execute("CREATE TABLE error_test (id INTEGER PRIMARY KEY, data TEXT UNIQUE)");
    conn->execute("INSERT INTO error_test VALUES (1, 'unique_value')");

    // Попытка вставить дубликат в уникальное поле — должно выбросить исключение
    BOOST_CHECK_THROW(
        conn->execute("INSERT INTO error_test VALUES (2, 'unique_value')"),
        std::runtime_error
    );

    // База данных должна остаться работоспособной после ошибки
    BOOST_CHECK_NO_THROW(conn->execute("INSERT INTO error_test VALUES (2, 'another_value')"));

    // Проверяем, что вторая вставка действительно прошла
    auto stmt = conn->prepareStatement("SELECT COUNT(*) FROM error_test");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 2); // Всего 2 записи (первая и корректная вторая)

    db->shutdown();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
