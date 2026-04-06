#pragma once
#include <memory>
#include <mutex>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteConnection;

class SqliteDatabase : public IDatabase
{
public:
    SqliteDatabase();
    ~SqliteDatabase() override;

    void initialize(const DatabaseConfig& config) override;
    void shutdown() override;

    std::unique_ptr<IConnection> getConnection() override;

    std::unique_ptr<IResultSet> query(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;

    void transaction(std::function<void()> callback) override;

    sqlite3* getHandle() const { return m_db; }

private:
    sqlite3* m_db = nullptr;
    DatabaseConfig m_config;
    bool m_initialized = false;
    std::mutex m_mutex;
};

} // namespace db::sqlite
