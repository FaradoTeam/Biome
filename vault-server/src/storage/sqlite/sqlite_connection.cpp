#include <filesystem>
#include <stdexcept>

#include <sqlite3.h>

#include "sqlite_connection.h"
#include "sqlite_statement.h"
#include "sqlite_transaction.h"

namespace
{
std::string normalizePath(const std::string& dbPath)
{
    // Нормализуем путь к файлу
    std::string result = dbPath;

    // Удаляем лишние слеши
    size_t pos;
    while ((pos = result.find("//")) != std::string::npos)
    {
        result.replace(pos, 2, "/");
    }

    // Удаляем префикс "./" если он есть
    if (result.find("./") == 0)
    {
        result = result.substr(2);
    }

    // Создаём директорию если нужно
    std::filesystem::path path(result);
    auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
    }

    return result;
}
} // namespace

namespace db
{

SqliteConnection::SqliteConnection(const std::string& dbPath)
{
    // Нормализуем путь к файлу
    const std::string normalizedPath = normalizePath(dbPath);

    const int rc = sqlite3_open(normalizedPath.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
        const std::string error = sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        m_db = nullptr;
        throw std::runtime_error("Failed to open database '" + normalizedPath + "': " + error);
    }
}

SqliteConnection::~SqliteConnection()
{
    if (m_db)
    {
        sqlite3_close(m_db);
    }
}

void SqliteConnection::checkError(int rc, const std::string& operation) const
{
    if (rc != SQLITE_OK
        && rc != SQLITE_ROW
        && rc != SQLITE_DONE)
    {
        throw std::runtime_error(
            operation + " failed: " + sqlite3_errmsg(m_db)
        );
    }
}

std::unique_ptr<IStatement> SqliteConnection::prepareStatement(
    const std::string& sql
)
{
    return std::make_unique<SqliteStatement>(*this, sql);
}

int64_t SqliteConnection::execute(const std::string& sql)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    char* errMsg = nullptr;
    const int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        const std::string error(errMsg ? errMsg : "Unknown error");
        sqlite3_free(errMsg);
        throw std::runtime_error("Execute failed: " + error);
    }

    return sqlite3_changes(m_db);
}

std::unique_ptr<ITransaction> SqliteConnection::beginTransaction()
{
    return std::make_unique<SqliteTransaction>(*this);
}

int64_t SqliteConnection::lastInsertId()
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return sqlite3_last_insert_rowid(m_db);
}

std::string SqliteConnection::escapeString(const std::string& value)
{
    struct EscapedString
    {
        char* ptr;
        EscapedString(char* p)
            : ptr(p)
        {
        }
        ~EscapedString()
        {
            if (ptr)
                sqlite3_free(ptr);
        }
        EscapedString(const EscapedString&) = delete;
        EscapedString& operator=(const EscapedString&) = delete;
        EscapedString(EscapedString&&) = delete;
        EscapedString& operator=(EscapedString&&) = delete;
    };

    EscapedString escaped(sqlite3_mprintf("%q", value.c_str()));
    if (!escaped.ptr)
        throw std::runtime_error("Failed to escape string");

    return std::string(escaped.ptr);
}

} // namespace db
