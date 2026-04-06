#pragma once
#include <memory>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteConnection;

/**
 * @brief Реализация ITransaction для SQLite.
 *
 * При создании автоматически выполняется BEGIN TRANSACTION.
 * Деструктор не вызывает rollback автоматически — это ответственность пользователя.
 */
class SqliteTransaction : public ITransaction
{
public:
    /**
     * @brief Конструктор.
     * @param conn Активное соединение (не владеет)
     * @note Сразу выполняет BEGIN TRANSACTION через conn->execute()
     */
    explicit SqliteTransaction(SqliteConnection* conn);
    ~SqliteTransaction() override;

    void commit() override; ///< Выполняет COMMIT
    void rollback() override; ///< Выполняет ROLLBACK
    bool isActive() const override { return m_active; }

private:
    SqliteConnection* m_conn; ///< Соединение (не владеет)
    bool m_active = true; ///< Активна ли транзакция
};

} // namespace db::sqlite
