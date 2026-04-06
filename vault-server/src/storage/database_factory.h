#pragma once
#include <memory>
#include <string>

#include "idatabase.h"

namespace db
{

enum class DatabaseType
{
    Sqlite,
    PostgreSQL, // еще не реализовано
};

class DatabaseFactory
{
public:
    static std::unique_ptr<IDatabase> create(DatabaseType type);
    static std::unique_ptr<IDatabase> createFromConnectionString(
        const std::string& connectionString
    );
};

} // namespace db
