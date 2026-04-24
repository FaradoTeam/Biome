#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "storage/idatabase.h"

struct sqlite3_stmt; // Forward declaration

namespace db
{

class SqliteConnection;

/**
 * @brief Реализация IResultSet для SQLite.
 *
 * @details Предоставляет доступ к результатам SELECT-запроса.
 *          Поддерживает итерацию по строкам и доступ к колонкам
 *          как по индексу, так и по имени.
 *
 * @note Важно: ResultSet привязан к подготовленному запросу (sqlite3_stmt)
 *       и не должен переживать его. Данные копируются при получении,
 *       поэтому ResultSet безопасен даже после закрытия statement'а.
 */
class SqliteResultSet final : public IResultSet
{
public:
    /**
     * @brief Конструктор.
     * @param connection Соединение с БД (нужно для мьютекса)
     * @param stmt Подготовленный запрос с результатом SELECT
     */
    explicit SqliteResultSet(SqliteConnection& connection, sqlite3_stmt* stmt);
    ~SqliteResultSet() override;

    // Запрещаем копирование, разрешаем перемещение
    SqliteResultSet(const SqliteResultSet&) = delete;
    SqliteResultSet& operator=(const SqliteResultSet&) = delete;
    SqliteResultSet(SqliteResultSet&& other) noexcept;
    SqliteResultSet& operator=(SqliteResultSet&& other) = delete;

    // Реализация IResultSet
    bool next() override;
    int columnCount() const override;
    std::string columnName(int index) const override;
    int columnIndex(const std::string& name) const override;
    bool isNull(int index) const override;
    FieldValue value(int index) const override;

private:
    /**
     * @brief Кэширует имена колонок и их индексы.
     *
     * @details Вызывается автоматически при первом обращении к именам колонок.
     *          Кэширование необходимо, так как SQLite не гарантирует
     *          неизменность имён колонок между вызовами.
     */
    void cacheColumnNames() const;

private:
    SqliteConnection& m_connection; ///< Ссылка на соединение
    sqlite3_stmt* m_stmt = { nullptr }; ///< Подготовленный запрос
    bool m_hasRow = false; ///< Есть ли текущая строка

    // Кэш для быстрого доступа к колонкам по имени
    mutable std::vector<std::string> m_columnNames; ///< Имена колонок в порядке индексов
    mutable std::unordered_map<std::string, int> m_columnIndexCache; ///< Индексы колонок по имени
};

} // namespace db
