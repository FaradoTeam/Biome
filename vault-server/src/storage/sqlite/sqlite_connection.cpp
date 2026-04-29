#include <filesystem>
#include <mutex>
#include <stdexcept>

#include <sqlite3.h>

#include "sqlite_connection.h"
#include "sqlite_statement.h"
#include "sqlite_transaction.h"

namespace
{
/**
 * @brief Нормализует путь к файлу БД.
 * @param dbPath Исходный путь
 * @return Нормализованный путь
 *
 * @details Выполняет следующие преобразования:
 *          - Удаляет повторяющиеся слеши ("//" -> "/")
 *          - Удаляет префикс "./"
 *          - Создаёт недостающие директории в пути
 */
std::string normalizePath(const std::string& dbPath)
{
    // Нормализуем путь к файлу
    std::string result = dbPath;

    // Удаляем лишние слеши (возникают при конкатенации путей)
    size_t pos;
    while ((pos = result.find("//")) != std::string::npos)
    {
        result.replace(pos, 2, "/");
    }

    // Удаляем префикс "./" если он есть
    if (result.find("./") == 0)
    {
        result = result.substr(2);
    }

    // Создаём директорию для файла БД, если её не существует
    // Это полезно, когда пользователь указал путь с несуществующими директориями
    std::filesystem::path path(result);
    auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::error_code ec; // Игнорируем ошибки - если создать не удалось,
        std::filesystem::create_directories(parent, ec); // то SQLite сам разберётся
    }

    return result;
}
} // namespace

namespace db
{

SqliteConnection::SqliteConnection(const std::string& dbPath)
{
    // Нормализуем путь к файлу (создаём директории, убираем артефакты)
    const std::string normalizedPath = normalizePath(dbPath);

    // Открываем (или создаём) базу данных SQLite
    const int rc = sqlite3_open(normalizedPath.c_str(), &m_db);
    if (rc != SQLITE_OK)
    {
        // Если открыть не удалось, получаем сообщение об ошибке
        const std::string error = sqlite3_errmsg(m_db);
        sqlite3_close(m_db); // Закрываем даже частично открытый хендл
        m_db = nullptr;
        throw std::runtime_error("Failed to open database '" + normalizedPath + "': " + error);
    }
}

SqliteConnection::~SqliteConnection()
{
    if (m_db)
    {
        // Закрываем соединение с БД
        // SQLite сам сбросит все несохранённые изменения и освободит ресурсы
        sqlite3_close(m_db);
    }
}

std::unique_ptr<IStatement> SqliteConnection::prepareStatement(
    const std::string& sql
)
{
    // Создаём подготовленный запрос (prepared statement)
    // SqliteStatement сам выполнит sqlite3_prepare_v2 в конструкторе
    return std::make_unique<SqliteStatement>(*this, sql);
}

int64_t SqliteConnection::execute(const std::string& sql)
{
    // Блокируем мьютекс на время выполнения (эксклюзивная блокировка для записи)
    ExclusiveLock lock(*this);

    char* errMsg = nullptr;
    // sqlite3_exec - удобная функция для выполнения SQL без обработки результатов
    const int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK)
    {
        const std::string error(errMsg ? errMsg : "Unknown error");
        sqlite3_free(errMsg); // Не забываем освободить сообщение об ошибке
        throw std::runtime_error("Execute failed: " + error);
    }

    // Возвращаем количество изменённых строк (для INSERT/UPDATE/DELETE)
    return sqlite3_changes(m_db);
}

std::unique_ptr<ITransaction> SqliteConnection::beginTransaction()
{
    // Создаём объект транзакции, который выполнит "BEGIN TRANSACTION" в конструкторе
    return std::make_unique<SqliteTransaction>(*this);
}

int64_t SqliteConnection::lastInsertId()
{
    // Для чтения достаточно shared_lock (разделяемая блокировка)
    SharedLock lock(*this);
    // Возвращаем ROWID последней вставленной записи
    return sqlite3_last_insert_rowid(m_db);
}

// TODO: Найти более подходящее место для этого метода.
std::string SqliteConnection::escapeString(const std::string& value)
{
    /**
     * @brief RAII-обёртка для автоматического освобождения памяти sqlite3_mprintf
     */
    struct EscapedString
    {
        char* ptr;
        EscapedString(char* p)
            : ptr(p)
        {
        }
        ~EscapedString()
        {
            if (ptr)
                sqlite3_free(ptr);
        }
        EscapedString(const EscapedString&) = delete;
        EscapedString& operator=(const EscapedString&) = delete;
        EscapedString(EscapedString&&) = delete;
        EscapedString& operator=(EscapedString&&) = delete;
    };

    // sqlite3_mprintf("%q") экранирует строку для безопасного использования в SQL
    // Заменяет ' на '' и делает другие необходимые экранирования
    EscapedString escaped(sqlite3_mprintf("%q", value.c_str()));
    if (!escaped.ptr)
        throw std::runtime_error("Failed to escape string");

    return std::string(escaped.ptr);
}

void SqliteConnection::checkError(int rc, const std::string& operation) const
{
    // Успешные коды: SQLITE_OK (успех), SQLITE_ROW (есть строка), SQLITE_DONE (запрос выполнен)
    if (rc != SQLITE_OK
        && rc != SQLITE_ROW
        && rc != SQLITE_DONE)
    {
        throw std::runtime_error(
            operation + " failed: " + sqlite3_errmsg(m_db)
        );
    }
}

} // namespace db
