#include <stdexcept>

#include "sqlite_result_set.h"
#include "sqlite_statement.h"

namespace db::sqlite
{

SqliteStatement::SqliteStatement(sqlite3* db, sqlite3_stmt* stmt)
    : m_db(db)
    , m_stmt(stmt)
{
    if (!m_stmt)
    {
        throw std::runtime_error("Invalid statement");
    }
}

SqliteStatement::~SqliteStatement()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
    }
}

int SqliteStatement::getParamIndex(const std::string& name)
{
    auto it = m_paramCache.find(name);
    if (it != m_paramCache.end())
    {
        return it->second;
    }

    int index = sqlite3_bind_parameter_index(m_stmt, name.c_str());
    if (index == 0)
    {
        throw std::runtime_error("Parameter not found: " + name);
    }

    m_paramCache[name] = index;
    return index;
}

void SqliteStatement::bindNull(const std::string& name)
{
    int index = getParamIndex(name);
    sqlite3_bind_null(m_stmt, index);
}

void SqliteStatement::bindInt64(const std::string& name, int64_t value)
{
    int index = getParamIndex(name);
    sqlite3_bind_int64(m_stmt, index, value);
}

void SqliteStatement::bindDouble(const std::string& name, double value)
{
    int index = getParamIndex(name);
    sqlite3_bind_double(m_stmt, index, value);
}

void SqliteStatement::bindString(const std::string& name, const std::string& value)
{
    int index = getParamIndex(name);
    sqlite3_bind_text(
        m_stmt,
        index,
        value.c_str(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT
    );
}

void SqliteStatement::bindBlob(const std::string& name, const Blob& value)
{
    int index = getParamIndex(name);
    sqlite3_bind_blob(
        m_stmt,
        index,
        value.data(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT
    );
}

void SqliteStatement::bindDateTime(const std::string& name, const DateTime& value)
{
    bindString(name, dateTimeToString(value));
}

int64_t SqliteStatement::execute()
{
    int rc = sqlite3_step(m_stmt);
    m_executed = true;

    if (rc == SQLITE_DONE)
    {
        sqlite3_reset(m_stmt);
        return sqlite3_changes(m_db);
    }
    else if (rc == SQLITE_ROW)
    {
        sqlite3_reset(m_stmt);
        throw std::runtime_error(
            "Statement returns rows, use executeQuery() instead"
        );
    }
    else
    {
        sqlite3_reset(m_stmt);
        throw std::runtime_error(
            "Execute failed: " + std::string(sqlite3_errmsg(m_db))
        );
    }
}

std::unique_ptr<IResultSet> SqliteStatement::executeQuery()
{
    if (m_executed)
    {
        reset();
    }

    return std::make_unique<SqliteResultSet>(m_stmt, false);
}

void SqliteStatement::reset()
{
    sqlite3_reset(m_stmt);
    sqlite3_clear_bindings(m_stmt);
    m_executed = false;
}

} // namespace db::sqlite
