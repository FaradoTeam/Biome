#pragma once

#include <memory>
#include <string>
#include <vector>

#include "idatabase.h"

struct sqlite3_stmt;

namespace db
{

class SqliteConnection;

class SqliteResultSet : public IResultSet
{
public:
    explicit SqliteResultSet(SqliteConnection& connection, sqlite3_stmt* stmt);
    ~SqliteResultSet() override;

    SqliteResultSet(const SqliteResultSet&) = delete;
    SqliteResultSet& operator=(const SqliteResultSet&) = delete;
    SqliteResultSet(SqliteResultSet&& other) noexcept;
    SqliteResultSet& operator=(SqliteResultSet&& other) = delete;

    bool next() override;
    int columnCount() const override;
    std::string columnName(int index) const override;
    int columnIndex(const std::string& name) const override;
    bool isNull(int index) const override;
    FieldValue value(int index) const override;

private:
    void cacheColumnNames() const;

private:
    SqliteConnection& m_connection;
    sqlite3_stmt* m_stmt = { nullptr };
    bool m_hasRow = false;
    mutable std::vector<std::string> m_columnNames;
    mutable std::unordered_map<std::string, int> m_columnIndexCache;
};

} // namespace db
