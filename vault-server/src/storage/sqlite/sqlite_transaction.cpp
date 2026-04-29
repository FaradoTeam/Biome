#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_transaction.h"

namespace db
{

SqliteTransaction::SqliteTransaction(SqliteConnection& conn)
    : m_connection(conn)
    , m_exclusiveLock(conn.mutex())
{
    // Начинаем транзакцию при создании объекта
    // SQLite автоматически выходит из autocommit-режима
    m_connection.isInTransaction = true;
    try
    {
        m_connection.execute("BEGIN TRANSACTION");
    }
    catch (...)
    {
        m_connection.isInTransaction = false;
        throw;
    }
}

SqliteTransaction::~SqliteTransaction()
{
    if (m_active)
    {
        try
        {
            // Если транзакция не была завершена явно, откатываем её
            // Это важно для предотвращения утечек блокировок БД
            rollback();
        }
        catch (...)
        {
            // Подавляем все исключения, так как мы в деструкторе
        }
    }
    m_connection.isInTransaction = false;
}

void SqliteTransaction::commit()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }

    // Фиксируем все изменения в БД
    m_connection.execute("COMMIT");
    m_active = false; // Транзакция завершена
}

void SqliteTransaction::rollback()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }

    // Отменяем все изменения с момента начала транзакции
    m_connection.execute("ROLLBACK");
    m_active = false; // Транзакция завершена
}

bool SqliteTransaction::isActive() const
{
    return m_active;
}

} // namespace db
