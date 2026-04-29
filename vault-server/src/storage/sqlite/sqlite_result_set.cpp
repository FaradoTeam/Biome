#include <cstring>
#include <stdexcept>

#include <sqlite3.h>

#include "sqlite_connection.h"
#include "sqlite_result_set.h"
#include "sqlite_statement.h"

namespace db
{

SqliteResultSet::SqliteResultSet(
    SqliteConnection& connection,
    sqlite3_stmt* stmt
)
    : m_connection(connection)
    , m_stmt(stmt)
{
    // Инициализация: запрос ещё не выполнен, строк нет
}

SqliteResultSet::~SqliteResultSet() = default;

SqliteResultSet::SqliteResultSet(SqliteResultSet&& other) noexcept
    : m_connection(other.m_connection)
    , m_stmt(other.m_stmt)
    , m_hasRow(other.m_hasRow)
    , m_columnNames(std::move(other.m_columnNames))
    , m_columnIndexCache(std::move(other.m_columnIndexCache))
{
    // Обнуляем исходный объект, чтобы он не пытался освободить ресурсы
    other.m_stmt = nullptr;
    other.m_hasRow = false;
}

bool SqliteResultSet::next()
{
    // Блокируем на чтение - несколько потоков могут читать одновременно,
    // но не должны писать (изменять состояние statement'а)
    SqliteConnection::SharedLock lock(m_connection);

    // Перемещаемся на следующую строку результата
    const int rc = sqlite3_step(m_stmt);
    if (rc == SQLITE_ROW)
    {
        m_hasRow = true;
        return true; // Есть ещё строки
    }
    else if (rc == SQLITE_DONE)
    {
        m_hasRow = false;
        return false; // Закончились строки
    }
    else
    {
        throw std::runtime_error("Failed to fetch row");
    }
}

int SqliteResultSet::columnCount() const
{
    return sqlite3_column_count(m_stmt);
}

std::string SqliteResultSet::columnName(int index) const
{
    cacheColumnNames();
    if (index < 0 || index >= static_cast<int>(m_columnNames.size()))
    {
        throw std::out_of_range("Column index out of range");
    }
    return m_columnNames[index];
}

int SqliteResultSet::columnIndex(const std::string& name) const
{
    cacheColumnNames();
    auto it = m_columnIndexCache.find(name);
    if (it == m_columnIndexCache.end())
    {
        throw std::runtime_error("Column not found: " + name);
    }
    return it->second;
}

bool SqliteResultSet::isNull(int index) const
{
    if (!m_hasRow)
    {
        throw std::runtime_error("No current row");
    }
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
}

FieldValue SqliteResultSet::value(int index) const
{
    if (!m_hasRow)
    {
        throw std::runtime_error("No current row");
    }

    if (index < 0 || index >= columnCount())
    {
        throw std::runtime_error(
            "Column index out of range: " + std::to_string(index)
        );
    }

    const int type = sqlite3_column_type(m_stmt, index);

    switch (type)
    {
    case SQLITE_INTEGER:
        // Целое число (64-битное)
        return FieldValue(
            static_cast<int64_t>(sqlite3_column_int64(m_stmt, index))
        );

    case SQLITE_FLOAT:
        // Число с плавающей запятой
        return FieldValue(sqlite3_column_double(m_stmt, index));

    case SQLITE_TEXT:
    {
        // Текст (строка)
        if (const unsigned char* text = sqlite3_column_text(m_stmt, index))
        {
            std::string strValue(reinterpret_cast<const char*>(text));

            // Умное преобразование: пытаемся распарсить как DateTime,
            // если строка выглядит как дата/время в формате "YYYY-MM-DD HH:MM:SS"
            if (strValue.length() >= 19
                && strValue[4] == '-'
                && strValue[7] == '-'
                && strValue[10] == ' '
                && strValue[13] == ':'
                && strValue[16] == ':')
            {
                try
                {
                    // Если успешно распарсилось - возвращаем как DateTime
                    return FieldValue(stringToDateTime(strValue));
                }
                catch (...)
                {
                    // Если парсинг не удался (например, невалидная дата),
                    // возвращаем как обычную строку
                    return FieldValue(std::move(strValue));
                }
            }

            // Обычная строка
            return FieldValue(std::move(strValue));
        }
        else
        {
            // Пустая строка = NULL
            return FieldValue();
        }
    }

    case SQLITE_BLOB:
    {
        // Бинарные данные
        const void* blob = sqlite3_column_blob(m_stmt, index);
        const int size = sqlite3_column_bytes(m_stmt, index);
        if (blob && size > 0)
        {
            Blob result(size);
            std::memcpy(result.data(), blob, size);
            return FieldValue(std::move(result));
        }
        return FieldValue(); // Пустой BLOB = NULL
    }

    case SQLITE_NULL:
    default:
        // NULL значение
        return FieldValue();
    }
}

void SqliteResultSet::cacheColumnNames() const
{
    if (!m_columnNames.empty())
    {
        return; // Уже закэшировано
    }

    int count = columnCount();
    m_columnNames.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        const char* name = sqlite3_column_name(m_stmt, i);
        m_columnNames.emplace_back(name ? name : "");
        m_columnIndexCache[m_columnNames.back()] = i;
    }
}

} // namespace db
