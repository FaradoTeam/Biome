#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

BOOST_AUTO_TEST_SUITE(SqliteTransactionTests)

/**
 * @brief Фикстура для тестов транзакций
 *
 * Создаёт таблицу bank_accounts (id, name, balance) с двумя тестовыми счетами:
 * - Alice: 1000
 * - Bob: 500
 */
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

        // Создаём таблицу счетов
        conn->execute(
            "CREATE TABLE tx_accounts ("
            "id INTEGER PRIMARY KEY, "
            "name TEXT, "
            "balance INTEGER)"
        );

        // Заполняем начальными данными
        conn->execute("INSERT INTO tx_accounts VALUES (1, 'Alice', 1000)");
        conn->execute("INSERT INTO tx_accounts VALUES (2, 'Bob', 500)");

        db.shutdown();

        // Переоткрываем для тестов
        db.initialize(config);
        conn = db.connection();
    }

    ~TransactionTestFixture()
    {
        conn.reset();
        std::error_code ec;
        std::filesystem::remove("./test_data/transaction_test.db", ec);
    }

    std::shared_ptr<IConnection> conn; ///< Соединение с БД для тестов
};

/**
 * @brief Проверка фиксации (commit) транзакции
 *
 * Изменения внутри транзакции должны стать видимыми после вызова commit().
 */
BOOST_FIXTURE_TEST_CASE(test_commit_changes, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    BOOST_CHECK(tx->isActive()); // Транзакция активна

    // Переводим 200 от Alice к Bob
    conn->execute("UPDATE tx_accounts SET balance = balance - 200 WHERE id = 1");
    conn->execute("UPDATE tx_accounts SET balance = balance + 200 WHERE id = 2");

    tx->commit(); // Фиксируем изменения
    BOOST_CHECK(!tx->isActive()); // После коммита транзакция больше не активна

    // Проверяем, что балансы изменились
    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = :id");

    stmt->bindInt64("id", 1);
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 800); // У Alice было 1000, стало 800

    stmt->reset();
    stmt->bindInt64("id", 2);
    rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 700); // У Bob было 500, стало 700
}

/**
 * @brief Проверка отката (rollback) транзакции
 *
 * Изменения внутри транзакции должны быть отменены после вызова rollback().
 */
BOOST_FIXTURE_TEST_CASE(test_rollback_changes, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();

    // Обнуляем оба счёта
    conn->execute("UPDATE tx_accounts SET balance = 0 WHERE id = 1");
    conn->execute("UPDATE tx_accounts SET balance = 0 WHERE id = 2");

    tx->rollback(); // Откатываем изменения
    BOOST_CHECK(!tx->isActive());

    // Балансы должны вернуться к исходным значениям
    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = :id");

    stmt->bindInt64("id", 1);
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1000); // Alice: было 1000, осталось 1000

    stmt->reset();
    stmt->bindInt64("id", 2);
    rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 500); // Bob: было 500, осталось 500
}

/**
 * @brief Проверка автоматического отката при разрушении объекта транзакции
 *
 * Если транзакция не была явно закоммичена, при выходе из области видимости
 * должен произойти автоматический откат (RAII-поведение).
 */
BOOST_FIXTURE_TEST_CASE(test_auto_rollback_on_destruction, TransactionTestFixture)
{
    {
        auto tx = conn->beginTransaction();
        conn->execute("UPDATE tx_accounts SET balance = 999 WHERE id = 1");
        // Транзакция не коммитится — при выходе из блока произойдёт откат
    } // tx уничтожается, вызывается rollback()

    // Баланс Alice должен остаться неизменным
    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 1000); // Изменения откатились
}

/**
 * @brief Проверка, что повторный commit() вызывает исключение
 *
 * Нельзя закоммитить уже завершённую транзакцию.
 */
BOOST_FIXTURE_TEST_CASE(test_commit_twice_throws, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->commit(); // Первый коммит успешен
    BOOST_CHECK_THROW(tx->commit(), std::runtime_error); // Второй — ошибка
}

/**
 * @brief Проверка, что повторный rollback() вызывает исключение
 *
 * Нельзя откатить уже завершённую транзакцию.
 */
BOOST_FIXTURE_TEST_CASE(test_rollback_twice_throws, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->rollback(); // Первый откат успешен
    BOOST_CHECK_THROW(tx->rollback(), std::runtime_error); // Второй — ошибка
}

/**
 * @brief Проверка, что после коммита транзакции можно выполнять обычные операции
 *
 * База данных должна оставаться в работоспособном состоянии.
 */
BOOST_FIXTURE_TEST_CASE(test_operations_after_commit_work, TransactionTestFixture)
{
    auto tx = conn->beginTransaction();
    tx->commit();

    // После коммита транзакции можно выполнять обычные запросы
    BOOST_CHECK_NO_THROW(conn->execute("UPDATE tx_accounts SET balance = 500 WHERE id = 1"));

    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 500);
}

/**
 * @brief Проверка, что вложенные транзакции не поддерживаются
 *
 * SQLite не поддерживает вложенные транзакции (savepoints не в счёт).
 * Попытка начать новую транзакцию при уже активной должна вызывать исключение.
 */
BOOST_FIXTURE_TEST_CASE(test_nested_transactions_not_supported, TransactionTestFixture)
{
    auto tx1 = conn->beginTransaction();
    BOOST_CHECK_THROW(conn->beginTransaction(), std::runtime_error); // Вложенная транзакция запрещена
    tx1->rollback();
}

/**
 * @brief Пример использования RAII-обёртки для автоматического управления транзакцией
 *
 * Демонстрирует паттерн, при котором транзакция автоматически откатывается
 * при исключении, но может быть явно закоммичена при успешном завершении.
 */
BOOST_FIXTURE_TEST_CASE(test_transaction_guard_raii, TransactionTestFixture)
{
    // Простая RAII-обёртка для демонстрации
    struct TransactionGuard
    {
        TransactionGuard(std::unique_ptr<ITransaction> tx)
            : m_tx(std::move(tx))
        {
        }

        ~TransactionGuard()
        {
            // Если транзакция всё ещё активна — откатываем (защита от утечек)
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
        guard.commit(); // Явно фиксируем успешную операцию
    } // guard разрушается, но транзакция уже закоммичена, повторный откат не происходит

    // Проверяем, что изменения сохранились
    auto stmt = conn->prepareStatement("SELECT balance FROM tx_accounts WHERE id = 1");
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueInt64(0), 3000);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace db::test
