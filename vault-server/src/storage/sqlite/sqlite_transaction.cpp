#include "sqlite_transaction.h"
#include "sqlite_connection.h"

namespace db::sqlite
{

SqliteTransaction::SqliteTransaction(SqliteConnection* conn)
    : m_conn(conn)
{
    if (!m_conn)
    {
        throw std::runtime_error("Invalid connection");
    }
    // Важно: транзакция считается активной сразу после создания.
    // BEGIN TRANSACTION уже был выполнен в SqliteConnection::beginTransaction().
    m_active = true;
}

SqliteTransaction::~SqliteTransaction()
{
    if (m_active)
    {
        try
        {
            // Если пользователь не вызвал commit() или rollback() явно,
            // автоматически откатываем транзакцию при разрушении объекта.
            // Это безопаснее, чем автоматический commit (может нарушить
            // целостность).
            rollback();
        }
        catch (...)
        {
            // Игнорируем исключения в деструкторе.
            // TODO: здесь хорошо бы залогировать ошибку.
        }
    }
}

void SqliteTransaction::commit()
{
    if (!m_active)
    {
        // Повторный commit или rollback запрещены.
        throw std::runtime_error("Transaction is not active");
    }

    // Фиксируем все изменения в БД.
    m_conn->execute("COMMIT");
    m_active = false; // Транзакция больше не активна.
}

void SqliteTransaction::rollback()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }

    // Отменяем все изменения, сделанные в рамках этой транзакции.
    m_conn->execute("ROLLBACK");
    m_active = false;
}

} // namespace db::sqlite
