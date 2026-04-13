#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "idatabase.h"

namespace db
{

class SqliteDatabase : public IDatabase
{
public:
    SqliteDatabase();
    ~SqliteDatabase() override;

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;
    SqliteDatabase(SqliteDatabase&&) = delete;
    SqliteDatabase& operator=(SqliteDatabase&&) = delete;

    void initialize(const DatabaseConfig& config) override;
    void shutdown() override;

    std::shared_ptr<IConnection> connection() override;

private:
    std::string m_dbPath;
    std::atomic<bool> m_initialized;
    std::weak_ptr<IConnection> m_activeConnection;
};

} // namespace db
