#include <filesystem>
#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"
#include "sqlite_result_set.h"

namespace db::sqlite
{

SqliteDatabase::SqliteDatabase() = default;

SqliteDatabase::~SqliteDatabase()
{
    try
    {
        shutdown();
    }
    catch (...)
    {
        // ignore
    }
}

// TODO: нужна корректировка метода
void SqliteDatabase::initialize(const DatabaseConfig& config)
{
    if (m_initialized)
    {
        throw std::runtime_error("Database already initialized");
    }

    m_config = config;

    std::string dbPath = "test.db";
    if (config.find("database") != config.end())
    {
        dbPath = config.at("database");
    }

    // Normalize path - удаляем лишние /./ и исправляем абсолютные пути
    std::string normalizedPath = dbPath;
    size_t pos;
    while ((pos = normalizedPath.find("/./")) != std::string::npos)
    {
        normalizedPath.replace(pos, 3, "/");
    }

    // Ensure directory exists for the database file
    std::filesystem::path fsPath(normalizedPath);
    std::filesystem::path parentDir = fsPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir))
    {
        std::error_code ec;
        std::filesystem::create_directories(parentDir, ec);
        if (ec)
        {
            // Если не удалось создать директорию, пробуем использовать относительный путь
            std::string fallbackPath = "./" + dbPath;
            std::filesystem::path fallbackFsPath(fallbackPath);
            std::filesystem::path fallbackParentDir = fallbackFsPath.parent_path();
            if (!fallbackParentDir.empty() && !std::filesystem::exists(fallbackParentDir))
            {
                std::filesystem::create_directories(fallbackParentDir, ec);
            }
            normalizedPath = fallbackPath;
        }
    }

    int rc = sqlite3_open(normalizedPath.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
        std::string error = "Failed to open database '" + normalizedPath + "': ";
        if (m_db)
        {
            error += sqlite3_errmsg(m_db);
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        else
        {
            error += sqlite3_errstr(rc);
        }
        throw std::runtime_error(error);
    }

    // Enable foreign keys - только если база данных инициализирована
    if (m_db)
    {
        char* errMsg = nullptr;
        int fk_rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (fk_rc != SQLITE_OK)
        {
            if (errMsg)
            {
                sqlite3_free(errMsg);
            }
        }
    }

    m_initialized = true;
}

void SqliteDatabase::shutdown()
{
    if (m_db)
    {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    m_initialized = false;
}

std::unique_ptr<IConnection> SqliteDatabase::getConnection()
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }
    return std::make_unique<SqliteConnection>(this);
}

std::unique_ptr<IResultSet> SqliteDatabase::query(const std::string& sql)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    sqlite3_stmt* stmt = nullptr;
    const char* tail = nullptr;

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, &tail);
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error("Query failed: " + std::string(sqlite3_errmsg(m_db)));
    }

    return std::make_unique<SqliteResultSet>(stmt, true);
}

int64_t SqliteDatabase::execute(const std::string& sql)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Execute failed: " + error);
    }

    return sqlite3_changes(m_db);
}

void SqliteDatabase::transaction(std::function<void()> callback)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    execute("BEGIN TRANSACTION");

    try
    {
        callback();
        execute("COMMIT");
    }
    catch (...)
    {
        execute("ROLLBACK");
        throw;
    }
}

} // namespace db::sqlite
