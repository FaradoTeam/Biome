#include <memory>
#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"

namespace db
{

SqliteDatabase::SqliteDatabase()
    : m_initialized(false)
{
    // Пустая инициализация - реальная настройка произойдёт в initialize()
}

SqliteDatabase::~SqliteDatabase()
{
    // Если БД была инициализирована, корректно завершаем работу
    if (m_initialized)
    {
        shutdown();
    }
}

void SqliteDatabase::initialize(const DatabaseConfig& config)
{
    if (m_initialized)
    {
        throw std::runtime_error("Database already initialized");
    }

    // Проверяем наличие обязательного параметра "database" (путь к файлу)
    auto it = config.find("database");
    if (it == config.end())
    {
        throw std::runtime_error("Database path not specified in config");
    }

    // Сохраняем путь к файлу БД
    m_dbPath = it->second;
    m_initialized = true;
}

void SqliteDatabase::shutdown()
{
    // Сбрасываем флаг инициализации
    m_initialized = false;

    // Освобождаем активное соединение, если оно ещё существует
    m_activeConnection.reset();
}

std::shared_ptr<IConnection> SqliteDatabase::connection()
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    std::lock_guard<std::mutex> lock(m_connectionMutex);

    // Проверяем, существует ли уже активное соединение
    if (auto connection = m_activeConnection.lock())
    {
        return connection;
    }

    // Создаём новое соединение
    auto result = std::make_shared<SqliteConnection>(m_dbPath);
    result->execute("PRAGMA journal_mode=WAL");
    result->execute("PRAGMA busy_timeout=5000");
    m_activeConnection = result;
    return result;
}

} // namespace db
