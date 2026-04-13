#include <stdexcept>

#include "sqlite_transaction.h"
#include "sqlite_connection.h"

namespace db
{

SqliteTransaction::SqliteTransaction(SqliteConnection& conn)
    : m_connection(conn)
{
    m_connection.execute("BEGIN TRANSACTION");
}

SqliteTransaction::~SqliteTransaction()
{
    if (m_active)
    {
        try
        {
            rollback();
        }
        catch (...)
        {
            // Деструктор не должен бросать исключений
        }
    }
}

void SqliteTransaction::commit()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }

    m_connection.execute("COMMIT");
    m_active = false;
}

void SqliteTransaction::rollback()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }

    m_connection.execute("ROLLBACK");
    m_active = false;
}

bool SqliteTransaction::isActive() const
{
    return m_active;
}

} // namespace db
