#pragma once
#include <memory>
#include <string>

#include "idatabase.h"

namespace db
{

/**
 * @brief Типы поддерживаемых СУБД.
 */
enum class DatabaseType
{
    Sqlite, ///< SQLite (встроенная БД)
    PostgreSQL, ///< PostgreSQL (ещё не реализовано)
};

/**
 * @brief Фабрика для создания экземпляров IDatabase.
 *
 * Позволяет создавать объекты БД по типу или по строке подключения.
 */
class DatabaseFactory
{
public:
    /**
     * @brief Создаёт базу данных указанного типа.
     * @param type Тип СУБД (например, DatabaseType::Sqlite)
     * @return Инициализированный объект IDatabase
     * @throws std::runtime_error если тип не поддерживается
     */
    static std::unique_ptr<IDatabase> create(DatabaseType type);

    /**
     * @brief Создаёт базу данных на основе строки подключения.
     * @param connectionString Строка вида "sqlite://path/to/db.db"
     * @return Инициализированный объект IDatabase
     * @throws std::runtime_error если формат строки не поддерживается
     */
    static std::unique_ptr<IDatabase> createFromConnectionString(
        const std::string& connectionString
    );
};

} // namespace db
