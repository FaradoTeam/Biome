#pragma once

#include <filesystem>
#include <iostream>
#include <memory>

#include <boost/test/unit_test.hpp>

#include "vault-server/src/storage/sqlite/sqlite_database.h"

namespace db::test
{

/**
 * @brief Базовый класс для фикстур с временной БД
 *
 * Автоматически создаёт временный файл базы данных перед тестом
 * и удаляет его после завершения. Предоставляет удобный доступ
 * к объекту базы данных и соединению.
 */
class TempDatabaseFixture
{
public:
    /**
     * @brief Конструктор фикстуры
     * @param dbName Уникальное имя файла БД (будет помещён в ./test_data/)
     *
     * Удаляет старый файл, если он существует, и инициализирует
     * новую базу данных SQLite с указанным именем.
     */
    TempDatabaseFixture(const std::string& dbName)
        : m_dbName(dbName)
        , m_dbPath("./test_data/" + dbName)
    {
        // Удаляем старый файл, если он остался от предыдущего запуска
        std::error_code ec;
        std::filesystem::remove(m_dbPath, ec);

        // Настраиваем и инициализируем базу данных
        DatabaseConfig config;
        config["database"] = m_dbPath;
        m_db.initialize(config);
    }

    /**
     * @brief Деструктор фикстуры
     *
     * Завершает работу с БД и удаляет временный файл.
     * Ошибки при очистке игнорируются, чтобы не нарушать выполнение тестов.
     */
    virtual ~TempDatabaseFixture()
    {
        try
        {
            m_db.shutdown();
            std::error_code ec;
            std::filesystem::remove(m_dbPath, ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке — тестовая среда не должна падать из-за этого
        }
    }

    // Доступ к объекту базы данных
    SqliteDatabase& getDatabase() { return m_db; }

    // Доступ к соединению с БД (удобно для выполнения запросов)
    std::shared_ptr<IConnection> connection() { return m_db.connection(); }

protected:
    std::string m_dbName; ///< Имя файла базы данных (без пути)
    std::string m_dbPath; ///< Полный путь к файлу БД
    SqliteDatabase m_db; ///< Экземпляр базы данных
};

/**
 * @brief Утилитарная функция для получения пути к тестовой БД
 * @param name Имя файла БД
 * @return Полный путь вида "./test_data/<name>"
 */
inline std::string getTestDbPath(const std::string& name)
{
    return "./test_data/" + name;
}

} // namespace db::test
