#include <filesystem>
#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"
#include "sqlite_result_set.h"

namespace db::sqlite
{

SqliteDatabase::SqliteDatabase() = default;
// Пустой конструктор: вся инициализация происходит в initialize().
// Это позволяет создавать объект до того, как станут известны параметры
// подключения.

SqliteDatabase::~SqliteDatabase()
{
    try
    {
        shutdown(); // Гарантированно закрываем соединение при разрушении.
    }
    catch (...)
    {
        // Игнорируем исключения в деструкторе — стандартная практика C++,
        // так как метание исключений из деструктора может привести
        // к terminate().
        // TODO: Добавить лог.
    }
}

void SqliteDatabase::initialize(const DatabaseConfig& config)
{
    // Проверяем, не инициализирована ли уже БД (повторная инициализация
    // запрещена).
    if (m_initialized)
    {
        throw std::runtime_error("Database already initialized");
    }

    m_config = config;

    // --- Определение пути к файлу базы данных ---
    std::string dbPath = "test.db"; // Значение по умолчанию (для тестирования).
    if (config.find("database") != config.end())
    {
        dbPath = config.at("database");
    }

    // --- Нормализация пути (удаляем артефакты типа "/./") ---
    std::string normalizedPath = dbPath;
    size_t pos;
    while ((pos = normalizedPath.find("/./")) != std::string::npos)
    {
        normalizedPath.replace(pos, 3, "/");
    }

    // --- Создание директории для файла БД, если её нет ---
    // SQLite не создаёт автоматически родительские директории, делаем это сами.
    std::filesystem::path fsPath(normalizedPath);
    std::filesystem::path parentDir = fsPath.parent_path();
    if (!parentDir.empty() && !std::filesystem::exists(parentDir))
    {
        // Используем error_code, чтобы не бросать исключения при проверке.
        std::error_code ec;
        std::filesystem::create_directories(parentDir, ec);
        if (ec)
        {
            // Если не удалось создать директорию (например, недостаточно прав),
            // пробуем использовать относительный путь в текущей директории.
            std::string fallbackPath = "./" + dbPath;
            std::filesystem::path fallbackFsPath(fallbackPath);
            std::filesystem::path fallbackParentDir = fallbackFsPath.parent_path();
            if (!fallbackParentDir.empty() && !std::filesystem::exists(fallbackParentDir))
            {
                std::filesystem::create_directories(fallbackParentDir, ec);
            }
            normalizedPath = fallbackPath;
        }
    }

    // --- Открытие (или создание) файла базы данных SQLite ---
    int rc = sqlite3_open(normalizedPath.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
        // Формируем понятное сообщение об ошибке.
        std::string error = "Failed to open database '" + normalizedPath + "': ";
        if (m_db)
        {
            error += sqlite3_errmsg(m_db);
            sqlite3_close(m_db); // Важно закрыть даже повреждённый дескриптор.
            m_db = nullptr;
        }
        else
        {
            error += sqlite3_errstr(rc);
        }
        throw std::runtime_error(error);
    }

    // --- Включение поддержки внешних ключей (foreign keys) ---
    // В SQLite внешние ключи по умолчанию выключены для обратной совместимости.
    // Включаем их принудительно для обеспечения целостности данных.
    if (m_db)
    {
        char* errMsg = nullptr;
        int fk_rc = sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (fk_rc != SQLITE_OK)
        {
            // TODO: Добавить логирование WARNING, т.к. ошибка PRAGMA
            // не критична для работы, поэтому не бросаем исключение.
            if (errMsg)
            {
                // Освобождаем память, даже если ошибку игнорируем.
                sqlite3_free(errMsg);
            }
        }
    }

    m_initialized = true;
}

void SqliteDatabase::shutdown()
{
    if (m_db)
    {
        sqlite3_close(m_db); // Закрываем соединение с БД.
        m_db = nullptr;
    }
    m_initialized = false;
}

std::unique_ptr<IConnection> SqliteDatabase::getConnection()
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }
    // Создаём новое соединение. Оно будет использовать этот же объект базы данных.
    // Важно: SQLite поддерживает многопоточность с одним соединением,
    // но для простоты мы создаём отдельные объекты-обёртки.
    return std::make_unique<SqliteConnection>(this);
}

std::unique_ptr<IResultSet> SqliteDatabase::query(const std::string& sql)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    // Защищаем критическую секцию: SQLite не является потокобезопасным
    // для операций с одним и тем же дескриптором из разных потоков.
    std::lock_guard<std::mutex> lock(m_mutex);

    sqlite3_stmt* stmt = nullptr;
    const char* tail = nullptr; // Не используется, но требуется sqlite3_prepare_v2.

    const int rc = sqlite3_prepare_v2(
        m_db,
        sql.c_str(),
        static_cast<int>(sql.size()),
        &stmt,
        &tail
    );
    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(
            "Query failed: " + std::string(sqlite3_errmsg(m_db))
        );
    }

    // autoFinalize = true: ResultSet сам удалит stmt, когда больше
    // не понадобится.
    return std::make_unique<SqliteResultSet>(stmt, true);
}

int64_t SqliteDatabase::execute(const std::string& sql)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    char* errMsg = nullptr;
    // sqlite3_exec — удобная функция для команд, не возвращающих данные.
    const int rc = sqlite3_exec(
        m_db,
        sql.c_str(),
        nullptr,
        nullptr,
        &errMsg
    );

    if (rc != SQLITE_OK)
    {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("Execute failed: " + error);
    }

    // Возвращаем количество изменённых строк (важно для UPDATE/DELETE).
    return sqlite3_changes(m_db);
}

void SqliteDatabase::transaction(std::function<void()> callback)
{
    if (!m_initialized)
    {
        throw std::runtime_error("Database not initialized");
    }

    // Начинаем транзакцию.
    execute("BEGIN TRANSACTION");

    try
    {
        callback(); // Выполняем пользовательский код.
        execute("COMMIT"); // Если всё успешно — фиксируем.
    }
    catch (...)
    {
        // При любом исключении откатываем изменения и пробрасываем
        // исключение дальше.
        execute("ROLLBACK");
        throw;
    }
}

} // namespace db::sqlite
