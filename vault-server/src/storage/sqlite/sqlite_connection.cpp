#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"
#include "sqlite_statement.h"
#include "sqlite_transaction.h"

namespace db::sqlite
{

SqliteConnection::SqliteConnection(SqliteDatabase* db)
    : m_db(db)
{
    if (!m_db)
    {
        throw std::runtime_error("Invalid database pointer");
    }

    m_isOpen = true;
}

SqliteConnection::~SqliteConnection()
{
    try
    {
        close();
    }
    catch (...)
    {
        // ignore
    }
}

void SqliteConnection::open(const std::string& /*connectionString*/)
{
    // NOTE: Строка подключения SQLite - это просто путь к базе данных
    // Основная база данных уже обрабатывает этот параметр
    // Этот параметр не используется, но сохраняется для обеспечения
    // совместимости интерфейса

    m_isOpen = true;
}

void SqliteConnection::close()
{
    m_isOpen = false;
}

bool SqliteConnection::isOpen() const
{
    return m_isOpen;
}

std::unique_ptr<IStatement> SqliteConnection::prepareStatement(const std::string& sql)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    sqlite3* handle = m_db->getHandle();
    sqlite3_stmt* stmt = nullptr;

    const char* tail = nullptr;
    int rc = sqlite3_prepare_v2(handle, sql.c_str(), static_cast<int>(sql.size()), &stmt, &tail);

    if (rc != SQLITE_OK)
    {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(handle)));
    }

    return std::make_unique<SqliteStatement>(handle, stmt);
}

int64_t SqliteConnection::execute(const std::string& sql)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }
    return m_db->execute(sql);
}

std::unique_ptr<IResultSet> SqliteConnection::executeQuery(const std::string& sql)
{
    return m_db->query(sql);
}

std::unique_ptr<ITransaction> SqliteConnection::beginTransaction()
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    execute("BEGIN TRANSACTION");
    return std::make_unique<SqliteTransaction>(this);
}

int64_t SqliteConnection::getLastInsertId()
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }
    return sqlite3_last_insert_rowid(m_db->getHandle());
}

std::string SqliteConnection::escapeString(const std::string& value)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    char* escaped = sqlite3_mprintf("%q", value.c_str());
    std::string result(escaped);
    sqlite3_free(escaped);
    return result;
}

} // namespace db::sqlite
