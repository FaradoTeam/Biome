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
        // Удаляем подготовленный запрос, освобождаем ресурсы.
        sqlite3_finalize(m_stmt);
    }
}

int SqliteStatement::getParamIndex(const std::string& name)
{
    // Проверяем кэш, чтобы не вызывать sqlite3_bind_parameter_index каждый раз.
    auto it = m_paramCache.find(name);
    if (it != m_paramCache.end())
    {
        return it->second;
    }

    // SQLite ищет параметр по имени (с учётом префиксов :, @, $).
    const int index = sqlite3_bind_parameter_index(m_stmt, name.c_str());
    if (index == 0)
    {
        throw std::runtime_error("Parameter not found: " + name);
    }

    // Сохраняем в кэш для будущих вызовов.
    m_paramCache[name] = index;
    return index;
}

void SqliteStatement::bindNull(const std::string& name)
{
    const int index = getParamIndex(name);
    sqlite3_bind_null(m_stmt, index);
}

void SqliteStatement::bindInt64(const std::string& name, int64_t value)
{
    const int index = getParamIndex(name);
    sqlite3_bind_int64(m_stmt, index, value);
}

void SqliteStatement::bindDouble(const std::string& name, double value)
{
    const int index = getParamIndex(name);
    sqlite3_bind_double(m_stmt, index, value);
}

void SqliteStatement::bindString(
    const std::string& name,
    const std::string& value
)
{
    const int index = getParamIndex(name);
    // SQLITE_TRANSIENT означает, что SQLite сделает копию строки,
    // так как мы не можем гарантировать, что value будет жить достаточно долго.
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

void SqliteStatement::bindDateTime(
    const std::string& name,
    const DateTime& value
)
{
    // DateTime храним в SQLite как текст в формате ISO8601.
    // Это не самый эффективный способ, но самый совместимый.
    bindString(name, dateTimeToString(value));
}

int64_t SqliteStatement::execute()
{
    // Выполняем подготовленный запрос (предполагается, что это
    // INSERT/UPDATE/DELETE).
    const int rc = sqlite3_step(m_stmt);
    m_executed = true;

    if (rc == SQLITE_DONE)
    {
        // Запрос успешно выполнен, сбрасываем состояние для возможного
        // повторного использования.
        sqlite3_reset(m_stmt);
        return sqlite3_changes(m_db); // Количество изменённых строк.
    }
    else if (rc == SQLITE_ROW)
    {
        // Запрос вернул строки, но пользователь вызвал execute()
        // вместо executeQuery(). Это ошибка использования API.
        sqlite3_reset(m_stmt);
        throw std::runtime_error(
            "Statement returns rows, use executeQuery() instead"
        );
    }
    else
    {
        // Любая другая ошибка (SQLITE_BUSY, SQLITE_LOCKED, SQLITE_CONSTRAINT
        // и т.д.).
        sqlite3_reset(m_stmt);
        throw std::runtime_error(
            "Execute failed: " + std::string(sqlite3_errmsg(m_db))
        );
    }
}

std::unique_ptr<IResultSet> SqliteStatement::executeQuery()
{
    // Если запрос уже выполнялся, сбрасываем его состояние (важно для
    // повторного использования).
    if (m_executed)
    {
        reset();
    }

    // autoFinalize = false: ResultSet НЕ будет удалять stmt,
    // потому что этим будет управлять SqliteStatement.
    // Это важно: владение stmt остаётся у этого объекта.
    return std::make_unique<SqliteResultSet>(m_stmt, false);
}

void SqliteStatement::reset()
{
    // Сбрасываем состояние запроса (как после sqlite3_step).
    sqlite3_reset(m_stmt);
    // Очищаем все привязанные значения.
    sqlite3_clear_bindings(m_stmt);
    m_executed = false;
}

} // namespace db::sqlite
