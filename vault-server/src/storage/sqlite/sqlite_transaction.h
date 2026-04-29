#pragma once

#include <mutex>
#include <shared_mutex>

#include "storage/idatabase.h"

namespace db
{

class SqliteConnection;

/**
 * @brief Реализация ITransaction для SQLite.
 *
 * @details Управляет транзакциями SQLite.
 *          Транзакция автоматически начинается в конструкторе
 *          и должна быть явно завершена commit() или rollback().
 *
 * @note В деструкторе выполняется rollback(), если транзакция ещё активна.
 *       Это гарантирует, что незавершённые транзакции не повиснут,
 *       но также означает, что нельзя полагаться на автоматический commit.
 */
class SqliteTransaction final : public ITransaction
{
public:
    /**
     * @brief Конструктор. Начинает новую транзакцию.
     * @param conn Соединение с БД
     * @throws std::runtime_error Если не удалось начать транзакцию
     */
    explicit SqliteTransaction(SqliteConnection& conn);
    ~SqliteTransaction() override;

    // Запрещаем копирование и перемещение
    SqliteTransaction(const SqliteTransaction&) = delete;
    SqliteTransaction& operator=(const SqliteTransaction&) = delete;
    SqliteTransaction(SqliteTransaction&&) = delete;
    SqliteTransaction& operator=(SqliteTransaction&&) = delete;

    // Реализация ITransaction
    void commit() override;
    void rollback() override;
    bool isActive() const override;

private:
    SqliteConnection& m_connection; ///< Ссылка на соединение
    std::unique_lock<std::shared_mutex> m_exclusiveLock; ///< Блокировка на запись
    bool m_active = true; ///< Активна ли транзакция (не завершена)
};

} // namespace db
