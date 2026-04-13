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

BOOST_AUTO_TEST_CASE(test_complete_workflow)
{
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);

    DatabaseConfig config;
    config["database"] = "./test_data/integration_test.db";
    db->initialize(config);

    auto conn = db->connection();

    // 1. Создание схемы
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
    auto insertStmt = conn->prepareStatement(
        "INSERT INTO products (name, price, stock) VALUES (:name, :price, :stock)"
    );

    std::vector<std::tuple<std::string, double, int>> products = {
        { "Laptop", 999.99, 10 },
        { "Mouse", 25.50, 50 },
        { "Keyboard", 75.00, 30 }
    };

    for (const auto& [name, price, stock] : products)
    {
        insertStmt->reset();
        insertStmt->bindString("name", name);
        insertStmt->bindDouble("price", price);
        insertStmt->bindInt64("stock", stock);
        int64_t result = insertStmt->execute();
        BOOST_CHECK_EQUAL(result, 1);
    }

    // 3. Выборка с фильтром
    auto queryStmt = conn->prepareStatement(
        "SELECT name, price, stock FROM products WHERE price > :min_price ORDER BY price"
    );
    queryStmt->bindDouble("min_price", 50.0);

    auto rs = queryStmt->executeQuery();

    std::vector<std::string> expectedNames = { "Keyboard", "Laptop" };
    std::vector<double> expectedPrices = { 75.00, 999.99 };

    int idx = 0;
    while (rs->next())
    {
        BOOST_CHECK_EQUAL(rs->valueString("name"), expectedNames[idx]);
        BOOST_CHECK_CLOSE(rs->valueDouble("price"), expectedPrices[idx], 0.01);
        ++idx;
    }
    BOOST_CHECK_EQUAL(idx, 2);

    // 4. Обновление в транзакции
    {
        auto tx = conn->beginTransaction();
        conn->execute("UPDATE products SET stock = stock - 1 WHERE name = 'Laptop'");
        conn->execute("UPDATE products SET stock = stock - 5 WHERE name = 'Mouse'");
        tx->commit();
    }

    // Проверяем обновление
    auto checkStmt = conn->prepareStatement("SELECT stock FROM products WHERE name = :name");

    checkStmt->bindString("name", "Laptop");
    rs = checkStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 9);

    checkStmt->reset();
    checkStmt->bindString("name", "Mouse");
    rs = checkStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 45);

    // 5. Удаление через подготовленный запрос
    auto deleteStmt = conn->prepareStatement("DELETE FROM products WHERE stock = :stock");
    deleteStmt->bindInt64("stock", 0);
    int64_t deleted = deleteStmt->execute();
    BOOST_CHECK_EQUAL(deleted, 0);

    // Финальная проверка
    auto countStmt = conn->prepareStatement("SELECT COUNT(*) FROM products");
    rs = countStmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 3);

    db->shutdown();
}

BOOST_AUTO_TEST_CASE(test_concurrent_operations)
{
    auto db = DatabaseFactory::create(DatabaseType::Sqlite);

    DatabaseConfig config;
    config["database"] = "./test_data/concurrent_test.db";
    db->initialize(config);

    // ОДНО соединение для всех потоков
    auto sharedConn = db->connection();

    sharedConn->execute("DROP TABLE IF EXISTS counter");
    sharedConn->execute("CREATE TABLE counter (id INTEGER PRIMARY KEY, value INTEGER)");
    sharedConn->execute("INSERT INTO counter VALUES (1, 0)");

    // Включаем многопоточный режим SQLite
    sharedConn->execute("PRAGMA journal_mode=WAL");
    sharedConn->execute("PRAGMA synchronous=NORMAL");

    std::vector<std::thread> threads;
    std::atomic<int> successes { 0 };
    std::mutex coutMutex;

    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back(
            [&sharedConn, &successes, &coutMutex]()
            {
                try
                {
                    int retries = 3;
                    while (retries-- > 0)
                    {
                        try
                        {
                            const int64_t result = sharedConn->execute(
                                "UPDATE counter SET value = value + 1 WHERE id = 1"
                            );

                            if (result > 0)
                            {
                                ++successes;
                            }
                            break;
                        }
                        catch (const std::exception& e)
                        {
                            if (retries == 0
                                || (std::string(e.what()).find("locked") == std::string::npos
                                    && std::string(e.what()).find("busy") == std::string::npos))
                            {
                                throw;
                            }
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

    for (auto& t : threads)
    {
        t.join();
    }

    auto checkStmt = sharedConn->prepareStatement("SELECT value FROM counter WHERE id = 1");
    auto rs = checkStmt->executeQuery();
    rs->next();
    int64_t finalValue = rs->valueInt64(0);

    BOOST_CHECK_EQUAL(finalValue, 5);
    BOOST_CHECK_EQUAL(successes, 5);

    db->shutdown();
}

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

    // Попытка вставить дубликат
    BOOST_CHECK_THROW(
        conn->execute("INSERT INTO error_test VALUES (2, 'unique_value')"),
        std::runtime_error
    );

    // База должна остаться работоспособной
    BOOST_CHECK_NO_THROW(conn->execute("INSERT INTO error_test VALUES (2, 'another_value')"));

    auto stmt = conn->prepareStatement("SELECT COUNT(*) FROM error_test");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 2);

    db->shutdown();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
