#pragma once
#include <string>
#include <unordered_map>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

/**
 * @brief Реализация IStatement для SQLite.
 *
 * Поддерживает именованные параметры (например, :id, :name)
 * и автоматическое кэширование индексов параметров.
 */
class SqliteStatement : public IStatement
{
public:
    SqliteStatement(sqlite3* db, sqlite3_stmt* stmt);
    ~SqliteStatement() override;

    int64_t execute() override;
    std::unique_ptr<IResultSet> executeQuery() override;

    void reset() override;

    // --- Привязка значений ---
    void bindNull(const std::string& name) override;
    void bindInt64(const std::string& name, int64_t value) override;
    void bindDouble(const std::string& name, double value) override;
    void bindString(const std::string& name, const std::string& value) override;
    void bindBlob(const std::string& name, const Blob& value) override;
    void bindDateTime(const std::string& name, const DateTime& value) override;

private:
    /**
     * @brief Возвращает индекс параметра по имени (с кэшированием).
     * @param name Имя параметра (например, ":id")
     * @return Индекс (начиная с 1)
     * @throws Если параметр не найден
     */
    int getParamIndex(const std::string& name);

private:
    sqlite3* m_db = nullptr; ///< Дескриптор БД
    sqlite3_stmt* m_stmt = nullptr; ///< Подготовленный запрос SQLite
    std::unordered_map<std::string, int> m_paramCache; ///< Кэш "имя → индекс"
    bool m_executed = false; ///< Был ли уже выполнен запрос
};

} // namespace db::sqlite
