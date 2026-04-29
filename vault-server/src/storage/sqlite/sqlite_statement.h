#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "storage/idatabase.h"

struct sqlite3_stmt; // Forward declaration для sqlite3_stmt (подготовленный запрос)

namespace db
{

class SqliteConnection;

/**
 * @brief Реализация IStatement для SQLite.
 *
 * @details Управляет подготовленным запросом SQLite.
 *          Поддерживает именованные параметры (с префиксом ':').
 *          Кэширует индексы параметров для повышения производительности.
 *
 * @note Подготовленные запросы в SQLite:
 *       - Безопасны (защита от SQL-инъекций)
 *       - Быстрее при многократном выполнении
 *       - Поддерживают BLOB и другие типы данных
 */
class SqliteStatement final : public IStatement
{
public:
    /**
     * @brief Конструктор. Подготавливает SQL-запрос.
     * @param conn Соединение с БД
     * @param sql Текст SQL-запроса с именованными параметрами
     * @throws std::runtime_error При ошибке подготовки запроса
     */
    SqliteStatement(SqliteConnection& conn, const std::string& sql);
    ~SqliteStatement() override;

    // Запрещаем копирование и перемещение
    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&&) = delete;
    SqliteStatement& operator=(SqliteStatement&&) = delete;

    // Реализация IStatement
    int64_t execute() override;
    std::unique_ptr<IResultSet> executeQuery() override;
    void reset() override;

    void bindNull(const std::string& name) override;
    void bindInt64(const std::string& name, int64_t value) override;
    void bindDouble(const std::string& name, double value) override;
    void bindString(const std::string& name, const std::string& value) override;
    void bindBlob(const std::string& name, const Blob& value) override;
    void bindDateTime(const std::string& name, const DateTime& value) override;

private:
    void unsafeReset();

    /**
     * @brief Получает индекс параметра по имени.
     * @param name Имя параметра (без префикса ':')
     * @return Индекс параметра (1-индексация в SQLite)
     * @throws std::runtime_error Если параметр не найден
     *
     * @details Результат кэшируется в m_paramIndexCache для быстрого доступа.
     *          SQLite использует 1-индексацию для параметров.
     */
    int getParamIndex(const std::string& name);

    /**
     * @brief Проверяет код ошибки SQLite и бросает исключение при необходимости.
     * @param rc Код возврата SQLite
     * @param operation Название операции для сообщения об ошибке
     */
    void checkError(int rc, const std::string& operation) const;

private:
    SqliteConnection& m_connection; ///< Ссылка на соединение
    sqlite3_stmt* m_stmt = { nullptr }; ///< Указатель на подготовленный запрос
    std::unordered_map<std::string, int> m_paramIndexCache; ///< Кэш: имя параметра -> индекс
};

} // namespace db
