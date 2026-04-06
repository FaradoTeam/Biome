#pragma once
#include <memory>
#include <mutex>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

/**
 * @brief Реализация IDatabase для SQLite.
 *
 * Управляет подключением к SQLite-файлу, потоками и основными операциями.
 * @note Использует мьютекс для потокобезопасности (SQLite в многопоточном режиме)
 */
class SqliteDatabase : public IDatabase
{
public:
    SqliteDatabase();
    ~SqliteDatabase() override;

    /**
     * @brief Инициализирует БД.
     * @param config Должен содержать ключ "database" с путём к файлу
     * @throws std::runtime_error при ошибке открытия/создания файла
     */
    void initialize(const DatabaseConfig& config) override;
    void shutdown() override;

    std::unique_ptr<IConnection> getConnection() override;

    std::unique_ptr<IResultSet> query(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;

    void transaction(std::function<void()> callback) override;

    /// Возвращает сырой указатель на sqlite3* (нужно для IConnection)
    sqlite3* getHandle() const { return m_db; }

private:
    sqlite3* m_db = nullptr; ///< Дескриптор SQLite
    DatabaseConfig m_config; ///< Сохранённая конфигурация
    bool m_initialized = false; ///< Флаг инициализации
    std::mutex m_mutex; ///< Мьютекс для потокобезопасности
};

} // namespace db::sqlite
