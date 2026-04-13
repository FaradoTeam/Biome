#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "idatabase.h"
#include "sqlite_result_set.h"

struct sqlite3;

namespace db
{

class SqliteStatement;

class SqliteConnection : public IConnection
{
public:
    explicit SqliteConnection(const std::string& dbPath);
    ~SqliteConnection() override;

    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;
    SqliteConnection(SqliteConnection&&) = delete;
    SqliteConnection& operator=(SqliteConnection&&) = delete;

    std::unique_ptr<IStatement> prepareStatement(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;

    std::unique_ptr<ITransaction> beginTransaction() override;

    int64_t lastInsertId() override;

    std::string escapeString(const std::string& value) override;

private:
    friend class SqliteStatement;
    friend class SqliteTransaction;
    friend class SqliteResultSet;

private:
    void checkError(int rc, const std::string& operation) const;
    sqlite3* handle() const { return m_db; }
    std::shared_mutex& mutex() { return m_mutex; }

private:
    sqlite3* m_db = { nullptr };
    mutable std::shared_mutex m_mutex;
};

} // namespace db
