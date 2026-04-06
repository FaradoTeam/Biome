#pragma once
#include <string>
#include <unordered_map>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteStatement : public IStatement
{
public:
    SqliteStatement(sqlite3* db, sqlite3_stmt* stmt);
    ~SqliteStatement() override;

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
    int getParamIndex(const std::string& name);

    sqlite3* m_db = nullptr;
    sqlite3_stmt* m_stmt = nullptr;
    std::unordered_map<std::string, int> m_paramCache;
    bool m_executed = false;
};

} // namespace db::sqlite
