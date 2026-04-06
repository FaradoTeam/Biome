#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <sqlite3.h>

#include "../idatabase.h"

namespace db::sqlite
{

class SqliteResultSet : public IResultSet
{
public:
    SqliteResultSet(sqlite3_stmt* stmt, bool autoFinalize);
    ~SqliteResultSet() override;

    bool next() override;

    int getColumnCount() const override;
    std::string getColumnName(int index) const override;
    int getColumnIndex(const std::string& name) const override;

    bool isNull(int index) const override;
    FieldValue getValue(int index) const override;

private:
    sqlite3_stmt* m_stmt = nullptr;
    bool m_autoFinalize;
    bool m_hasRow;
    bool m_isExecuted;
    mutable std::unordered_map<std::string, int> m_columnCache;
};

} // namespace db::sqlite
