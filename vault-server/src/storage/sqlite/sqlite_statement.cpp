#include <sqlite3.h>
#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_result_set.h"
#include "sqlite_statement.h"

namespace db
{

SqliteStatement::SqliteStatement(SqliteConnection& conn, const std::string& sql)
    : m_connection(conn)
{
    std::shared_lock<std::shared_mutex> lock(m_connection.m_mutex);

    const int rc = sqlite3_prepare_v2(
        m_connection.m_db,
        sql.c_str(),
        -1,
        &m_stmt,
        nullptr
    );

    checkError(rc, "Prepare statement");
}

SqliteStatement::~SqliteStatement()
{
    if (m_stmt)
    {
        sqlite3_finalize(m_stmt);
    }
}

void SqliteStatement::checkError(int rc, const std::string& operation) const
{
    if (rc != SQLITE_OK
        && rc != SQLITE_ROW
        && rc != SQLITE_DONE)
    {
        throw std::runtime_error(
            operation + " failed: " + sqlite3_errmsg(m_connection.m_db)
        );
    }
}

int SqliteStatement::getParamIndex(const std::string& name)
{
    auto it = m_paramIndexCache.find(name);
    if (it != m_paramIndexCache.end())
    {
        return it->second;
    }

    const std::string paramName = ":" + name;
    const int index = sqlite3_bind_parameter_index(m_stmt, paramName.c_str());
    if (index == 0)
    {
        throw std::runtime_error("Parameter not found: " + name);
    }

    m_paramIndexCache[name] = index;
    return index;
}

int64_t SqliteStatement::execute()
{
    std::unique_lock<std::shared_mutex> lock(m_connection.m_mutex);

    const int rc = sqlite3_step(m_stmt);
    checkError(rc, "Execute statement");

    const int64_t changes = sqlite3_changes(m_connection.m_db);
    reset();

    return changes;
}

std::unique_ptr<IResultSet> SqliteStatement::executeQuery()
{
    return std::make_unique<SqliteResultSet>(m_connection, m_stmt);
}

void SqliteStatement::reset()
{
    int rc = sqlite3_reset(m_stmt);
    checkError(rc, "Reset statement");

    rc = sqlite3_clear_bindings(m_stmt);
    checkError(rc, "Clear bindings");
}

void SqliteStatement::bindNull(const std::string& name)
{
    const int index = getParamIndex(name);
    const int rc = sqlite3_bind_null(m_stmt, index);
    checkError(rc, "Bind null");
}

void SqliteStatement::bindInt64(const std::string& name, int64_t value)
{
    const int index = getParamIndex(name);
    const int rc = sqlite3_bind_int64(m_stmt, index, value);
    checkError(rc, "Bind int64");
}

void SqliteStatement::bindDouble(const std::string& name, double value)
{
    const int index = getParamIndex(name);
    const int rc = sqlite3_bind_double(m_stmt, index, value);
    checkError(rc, "Bind double");
}

void SqliteStatement::bindString(const std::string& name, const std::string& value)
{
    const int index = getParamIndex(name);
    const int rc = sqlite3_bind_text(
        m_stmt,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT
    );
    checkError(rc, "Bind string");
}

void SqliteStatement::bindBlob(const std::string& name, const Blob& value)
{
    const int index = getParamIndex(name);
    const int rc = sqlite3_bind_blob(
        m_stmt,
        index,
        value.data(),
        value.size(),
        SQLITE_TRANSIENT
    );
    checkError(rc, "Bind blob");
}

void SqliteStatement::bindDateTime(
    const std::string& name,
    const DateTime& value
)
{
    bindString(name, dateTimeToString(value));
}

} // namespace db
