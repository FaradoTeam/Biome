#include <stdexcept>

#include <sqlite3.h>

#include "sqlite_connection.h"
#include "sqlite_result_set.h"
#include "sqlite_statement.h"

namespace db
{

SqliteStatement::SqliteStatement(SqliteConnection& conn, const std::string& sql)
    : m_connection(conn)
{
    // Для подготовки запроса достаточно shared_lock (чтение)
    SqliteConnection::SharedLock lock(m_connection);

    // Подготавливаем запрос - SQLite парсит SQL и создаёт внутреннее представление
    const int rc = sqlite3_prepare_v2(
        m_connection.m_db, // Соединение с БД
        sql.c_str(), // Текст SQL
        -1, // Длина (-1 = до завершающего нуля)
        &m_stmt, // Результат - подготовленный запрос
        nullptr // Указатель на остаток строки (не нужен)
    );

    checkError(rc, "Prepare statement");
}

SqliteStatement::~SqliteStatement()
{
    if (m_stmt)
    {
        SqliteConnection::ExclusiveLock lock(m_connection);
        // Финализируем запрос - освобождаем все ресурсы
        // Это необходимо делать для каждого подготовленного запроса
        sqlite3_finalize(m_stmt);
    }
}

int64_t SqliteStatement::execute()
{
    // Для выполнения нужна эксклюзивная блокировка (запись)
    SqliteConnection::ExclusiveLock lock(m_connection);

    // Выполняем запрос
    const int rc = sqlite3_step(m_stmt);
    checkError(rc, "Execute statement");

    // Получаем количество изменённых строк
    const int64_t changes = sqlite3_changes(m_connection.m_db);

    // Сбрасываем состояние запроса для возможности повторного использования
    unsafeReset();

    return changes;
}

std::unique_ptr<IResultSet> SqliteStatement::executeQuery()
{
    // Не блокируем здесь - блокировка будет в SqliteResultSet::next()
    // Возвращаем результат выполнения SELECT-запроса
    return std::make_unique<SqliteResultSet>(m_connection, m_stmt);
}

void SqliteStatement::reset()
{
    SqliteConnection::ExclusiveLock lock(m_connection);

    // Сбрасываем запрос в состояние "готов к выполнению"
    int rc = sqlite3_reset(m_stmt);
    checkError(rc, "Reset statement");

    // Очищаем все привязки параметров
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
    // SQLITE_TRANSIENT означает, что SQLite сделает копию строки
    // (безопасно, даже если строка будет уничтожена после вызова)
    const int rc = sqlite3_bind_text(
        m_stmt,
        index,
        value.c_str(),
        -1, // Длина (-1 = до завершающего нуля)
        SQLITE_TRANSIENT // Скопировать данные
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
        SQLITE_TRANSIENT // Скопировать данные
    );
    checkError(rc, "Bind blob");
}

void SqliteStatement::bindDateTime(
    const std::string& name,
    const DateTime& value
)
{
    // Преобразуем DateTime в строку ISO8601 и связываем как строку
    // SQLite не имеет встроенного типа DateTime, поэтому храним как TEXT
    bindString(name, dateTimeToString(value));
}

void SqliteStatement::unsafeReset()
{
    // Сбрасываем запрос в состояние "готов к выполнению"
    int rc = sqlite3_reset(m_stmt);
    checkError(rc, "Reset statement");

    // Очищаем все привязки параметров
    rc = sqlite3_clear_bindings(m_stmt);
    checkError(rc, "Clear bindings");
}


int SqliteStatement::getParamIndex(const std::string& name)
{
    // Проверяем кэш
    auto it = m_paramIndexCache.find(name);
    if (it != m_paramIndexCache.end())
    {
        return it->second;
    }

    // SQLite ожидает параметры с префиксом ':'
    const std::string paramName = ":" + name;
    const int index = sqlite3_bind_parameter_index(m_stmt, paramName.c_str());
    if (index == 0)
    {
        throw std::runtime_error("Parameter not found: " + name);
    }

    // Сохраняем в кэш для будущих вызовов
    m_paramIndexCache[name] = index;
    return index;
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

} // namespace db
