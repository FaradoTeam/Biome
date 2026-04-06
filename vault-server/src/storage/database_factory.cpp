#include "database_factory.h"

#include "sqlite/sqlite_database.h"

namespace db
{

std::unique_ptr<IDatabase> DatabaseFactory::create(DatabaseType type)
{
    switch (type)
    {
    case DatabaseType::Sqlite:
        return std::make_unique<sqlite::SqliteDatabase>();
    case DatabaseType::PostgreSQL:
        throw std::runtime_error("PostgreSQL not implemented yet");
    default:
        throw std::runtime_error("Unknown database type");
    }
}

std::unique_ptr<IDatabase> DatabaseFactory::createFromConnectionString(
    const std::string& connectionString
)
{
    if (connectionString.find("sqlite://") == 0)
    {
        auto db = std::make_unique<sqlite::SqliteDatabase>();
        DatabaseConfig config;
        config["database"] = connectionString.substr(8);
        db->initialize(config);
        return db;
    }
    throw std::runtime_error(
        "Unsupported connection string: " + connectionString
    );
}

} // namespace db
