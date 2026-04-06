#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

/**
 * @brief Реализация IResultSet для SQLite.
 *
 * Итерируется по строкам результата SELECT, автоматически преобразует
 * типы SQLite в типы C++ (включая парсинг DateTime из строк).
 */
class SqliteResultSet : public IResultSet
{
public:
    /**
     * @brief Конструктор.
     * @param stmt Подготовленный запрос SQLite (уже выполненный через sqlite3_step)
     * @param autoFinalize Если true — удалит stmt в деструкторе, иначе только сбросит
     */
    SqliteResultSet(sqlite3_stmt* stmt, bool autoFinalize);
    ~SqliteResultSet() override;

    bool next() override;

    int getColumnCount() const override;
    std::string getColumnName(int index) const override;
    int getColumnIndex(const std::string& name) const override;

    bool isNull(int index) const override;
    FieldValue getValue(int index) const override;

private:
    sqlite3_stmt* m_stmt = nullptr; ///< SQLite statement
    bool m_autoFinalize; ///< Удалять ли statement в конце
    bool m_hasRow; ///< Есть ли текущая строка
    bool m_isExecuted; ///< Был ли выполнен sqlite3_step хотя бы раз

    /// Кэш для ускорения поиска индекса по имени колонки
    mutable std::unordered_map<std::string, int> m_columnCache;
};

} // namespace db::sqlite
