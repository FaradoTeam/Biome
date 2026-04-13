#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "idatabase.h"

struct sqlite3_stmt;

namespace db
{

class SqliteConnection;

class SqliteStatement : public IStatement
{
public:
    SqliteStatement(SqliteConnection& conn, const std::string& sql);
    ~SqliteStatement() override;

    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&&) = delete;
    SqliteStatement& operator=(SqliteStatement&&) = delete;

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
    void checkError(int rc, const std::string& operation) const;

private:
    SqliteConnection& m_connection;
    sqlite3_stmt* m_stmt = { nullptr };
    std::unordered_map<std::string, int> m_paramIndexCache;
};

} // namespace db
