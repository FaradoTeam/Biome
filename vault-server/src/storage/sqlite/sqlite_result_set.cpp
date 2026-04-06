#include <stdexcept>

#include "sqlite_result_set.h"

namespace db::sqlite
{

SqliteResultSet::SqliteResultSet(sqlite3_stmt* stmt, bool autoFinalize)
    : m_stmt(stmt)
    , m_autoFinalize(autoFinalize)
    , m_hasRow(false)
    , m_isExecuted(false)
{
    if (!m_stmt)
    {
        throw std::runtime_error("Invalid statement");
    }
}

SqliteResultSet::~SqliteResultSet()
{
    if (m_autoFinalize && m_stmt)
    {
        // Режим владения: мы сами создали stmt и должны его удалить.
        sqlite3_finalize(m_stmt);
    }
    else if (m_stmt && !m_autoFinalize)
    {
        // Режим невладения: просто сбрасываем запрос, чтобы его можно было
        // использовать снова.
        sqlite3_reset(m_stmt);
    }
}

bool SqliteResultSet::next()
{
    if (!m_stmt)
    {
        return false;
    }

    // Перемещаем курсор на следующую строку результата.
    const int rc = sqlite3_step(m_stmt);
    m_isExecuted = true;

    if (rc == SQLITE_ROW)
    {
        // Есть ещё строки.
        m_hasRow = true;
        return true;
    }
    else if (rc == SQLITE_DONE)
    {
        // Больше нет строк.
        m_hasRow = false;
        return false;
    }
    else
    {
        // Ошибка (SQLITE_BUSY, SQLITE_ERROR и т.д.).
        throw std::runtime_error(
            "ResultSet next failed: "
            + std::string(sqlite3_errmsg(sqlite3_db_handle(m_stmt)))
        );
    }
}

int SqliteResultSet::getColumnCount() const
{
    if (!m_stmt)
    {
        throw std::runtime_error("ResultSet is not valid");
    }
    return sqlite3_column_count(m_stmt);
}

std::string SqliteResultSet::getColumnName(int index) const
{
    if (!m_stmt)
    {
        throw std::runtime_error("ResultSet is not valid");
    }

    const int count = sqlite3_column_count(m_stmt);
    if (index < 0 || index >= count)
    {
        throw std::runtime_error(
            "Invalid column index: "
            + std::to_string(index) + ", max: " + std::to_string(count - 1)
        );
    }

    const char* name = sqlite3_column_name(m_stmt, index);
    if (!name)
    {
        throw std::runtime_error(
            "Failed to get column name for index: " + std::to_string(index)
        );
    }
    return std::string(name);
}

int SqliteResultSet::getColumnIndex(const std::string& name) const
{
    // Проверяем кэш для быстрого доступа.
    auto it = m_columnCache.find(name);
    if (it != m_columnCache.end())
    {
        return it->second;
    }

    // Линейный поиск по именам колонок.
    const int count = getColumnCount();
    for (int i = 0; i < count; ++i)
    {
        if (getColumnName(i) == name)
        {
            m_columnCache[name] = i;
            return i;
        }
    }

    throw std::runtime_error("Column not found: " + name);
}

bool SqliteResultSet::isNull(int index) const
{
    if (!m_stmt || !m_hasRow)
    {
        throw std::runtime_error("No current row or invalid statement");
    }
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
}

FieldValue SqliteResultSet::getValue(int index) const
{
    if (!m_stmt)
    {
        throw std::runtime_error("ResultSet is not valid");
    }

    if (!m_hasRow)
    {
        throw std::runtime_error("No current row. Call next() first.");
    }

    int count = sqlite3_column_count(m_stmt);
    if (index < 0 || index >= count)
    {
        throw std::runtime_error(
            "Invalid column index: "
            + std::to_string(index) + ", max: " + std::to_string(count - 1)
        );
    }

    const int type = sqlite3_column_type(m_stmt, index);

    switch (type)
    {
    case SQLITE_NULL:
    {
        return FieldValue(nullptr);
    }

    case SQLITE_INTEGER:
    {
        sqlite3_int64 val = sqlite3_column_int64(m_stmt, index);
        return FieldValue(static_cast<int64_t>(val));
    }

    case SQLITE_FLOAT:
    {
        double val = sqlite3_column_double(m_stmt, index);
        return FieldValue(val);
    }

    case SQLITE_TEXT:
    {
        const char* text = reinterpret_cast<const char*>(
            sqlite3_column_text(m_stmt, index)
        );
        if (text)
        {
            std::string strValue(text);

            // Если строка похожа на дату/время — парсим её.
            // Формат ожидается: "YYYY-MM-DD HH:MM:SS" (ровно 19 символов).
            if (strValue.length() == 19
                && strValue[4] == '-'
                && strValue[7] == '-'
                && strValue[10] == ' '
                && strValue[13] == ':'
                && strValue[16] == ':')
            {
                try
                {
                    return FieldValue(stringToDateTime(strValue));
                }
                catch (...)
                {
                    // Если парсинг не удался (например, невалидная дата),
                    // возвращаем как обычную строку — это безопаснее,
                    // чем бросать исключение.
                    return FieldValue(strValue);
                }
            }
            return FieldValue(strValue);
        }
        return FieldValue(std::string()); // Пустая строка, не NULL.
    }

    case SQLITE_BLOB:
    {
        const void* blob = sqlite3_column_blob(m_stmt, index);
        int size = sqlite3_column_bytes(m_stmt, index);
        if (blob && size > 0)
        {
            const uint8_t* bytes = static_cast<const uint8_t*>(blob);
            return FieldValue(Blob(bytes, bytes + size));
        }
        return FieldValue(Blob()); // Пустой BLOB.
    }

    default:
        throw std::runtime_error("Unknown SQLite type");
    }
}

} // namespace db::sqlite
