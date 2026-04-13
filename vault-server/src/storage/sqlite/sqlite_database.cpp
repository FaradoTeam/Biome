#include <memory>
#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"

namespace db
{

SqliteDatabase::SqliteDatabase()
    : m_initialized(false)
{
}

SqliteDatabase::~SqliteDatabase()
{
    if (m_initialized)
    {
        shutdown();
    }
}

void SqliteDatabase::initialize(const DatabaseConfig& config)
{
    if (m_initialized)
    {
        throw std::runtime_error("Database already initialized");
    }

    auto it = config.find("database");
    if (it == config.end())
    {
        throw std::runtime_error("Database path not specified in config");
    }

    m_dbPath = it->second;
    m_initialized = true;
}

void SqliteDatabase::shutdown()
{
    m_initialized = false;
    m_activeConnection.reset();
}

std::shared_ptr<IConnection> SqliteDatabase::connection()
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    if (m_activeConnection.lock())
    {
        throw std::runtime_error(
            "SQLite does not support multiple connections. "
            "Only one connection can be active at a time."
        );
    }

    auto result = std::make_shared<SqliteConnection>(m_dbPath);
    m_activeConnection = result;
    return result;
}

} // namespace db
