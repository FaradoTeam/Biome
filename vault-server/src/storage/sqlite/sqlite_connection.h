#pragma once

#include <cstddef>
#include <memory>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteDatabase;

class SqliteConnection : public IConnection
{
public:
    explicit SqliteConnection(SqliteDatabase* db);
    ~SqliteConnection() override;

    void open(const std::string& connectionString) override;
    void close() override;
    bool isOpen() const override;

    std::unique_ptr<IStatement> prepareStatement(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;

    std::unique_ptr<IResultSet> executeQuery(const std::string& sql);
    std::unique_ptr<ITransaction> beginTransaction() override;

    int64_t getLastInsertId() override;
    std::string escapeString(const std::string& value) override;

private:
    SqliteDatabase* m_db = nullptr;
    bool m_isOpen = false;
};

} // namespace db::sqlite
