#include "database_factory.h"

#include "sqlite/sqlite_database.h"

namespace db
{

std::unique_ptr<IDatabase> DatabaseFactory::create(DatabaseType type)
{
    switch (type)
    {
    case DatabaseType::Sqlite:
        // Создаём экземпляр SQLite-базы данных.
        // Важно: объект ещё не инициализирован, initialize() будет вызван позже.
        return std::make_unique<SqliteDatabase>();

    case DatabaseType::PostgreSQL:
        // PostgreSQL пока не поддерживается, но заглушка оставлена
        // для будущего расширения.
        throw std::runtime_error("PostgreSQL not implemented yet");

    default:
        // Защита от добавления новых типов без обновления фабрики.
        throw std::runtime_error("Unknown database type");
    }
}

std::unique_ptr<IDatabase> DatabaseFactory::createFromConnectionString(
    const std::string& connectionString
)
{
    // Формат строки подключения: "sqlite://путь/к/файлу.db"
    // Проверяем префикс для определения типа БД.
    if (connectionString.find("sqlite://") == 0)
    {
        auto db = std::make_unique<SqliteDatabase>();
        DatabaseConfig config;

        // Извлекаем путь к файлу, отрезая префикс "sqlite://".
        config["database"] = connectionString.substr(9);

        // Сразу инициализируем базу данных, чтобы пользователь получил готовый объект.
        db->initialize(config);
        return db;
    }

    // Если префикс не распознан — бросаем исключение с пояснением.
    throw std::runtime_error(
        "Unsupported connection string: " + connectionString
    );
}

} // namespace db
