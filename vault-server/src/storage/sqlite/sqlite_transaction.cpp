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
            // игнорируем в деструкторе
        }
    }
}

void SqliteTransaction::commit()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }
    m_conn->execute("COMMIT");
    m_active = false;
}

void SqliteTransaction::rollback()
{
    if (!m_active)
    {
        throw std::runtime_error("Transaction is not active");
    }
    m_conn->execute("ROLLBACK");
    m_active = false;
}

} // namespace db::sqlite
