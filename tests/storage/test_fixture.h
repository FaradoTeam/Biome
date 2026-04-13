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
 */
class TempDatabaseFixture
{
public:
    TempDatabaseFixture(const std::string& dbName)
        : m_dbName(dbName)
        , m_dbPath("./test_data/" + dbName)
    {
        std::error_code ec;
        std::filesystem::remove(m_dbPath, ec);

        DatabaseConfig config;
        config["database"] = m_dbPath;
        m_db.initialize(config);
    }

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
            // Игнорируем ошибки при очистке
        }
    }

    SqliteDatabase& getDatabase() { return m_db; }
    std::shared_ptr<IConnection> getConnection() { return m_db.connection(); }

protected:
    std::string m_dbName;
    std::string m_dbPath;
    SqliteDatabase m_db;
};

inline std::string getTestDbPath(const std::string& name)
{
    return "./test_data/" + name;
}

} // namespace db::test
