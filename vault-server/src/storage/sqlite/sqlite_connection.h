#pragma once
#include <cstddef>
#include <memory>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteDatabase;

/**
 * @brief Реализация IConnection для SQLite.
 *
 * Предоставляет методы для работы с соединением, подготовки запросов
 * и транзакций. Каждый экземпляр использует общий SqliteDatabase.
 */
class SqliteConnection : public IConnection
{
public:
    /**
     * @brief Конструктор.
     * @param db Указатель на родительский объект базы данных (не владеет)
     */
    explicit SqliteConnection(SqliteDatabase* db);
    ~SqliteConnection() override;

    void open(const std::string& connectionString) override; // Для SQLite — no-op
    void close() override;
    bool isOpen() const override;

    std::unique_ptr<IStatement> prepareStatement(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;

    /// Вспомогательный метод для выполнения SELECT (используется внутри)
    std::unique_ptr<IResultSet> executeQuery(const std::string& sql);
    std::unique_ptr<ITransaction> beginTransaction() override;

    int64_t getLastInsertId() override;
    std::string escapeString(const std::string& value) override;

private:
    SqliteDatabase* m_db = nullptr; ///< Родительская БД (не владеет)
    bool m_isOpen = false; ///< Флаг открытого соединения
};

} // namespace db::sqlite
