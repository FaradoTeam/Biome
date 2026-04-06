#include <stdexcept>

#include "sqlite_connection.h"
#include "sqlite_database.h"
#include "sqlite_statement.h"
#include "sqlite_transaction.h"

namespace db::sqlite
{

SqliteConnection::SqliteConnection(SqliteDatabase* db)
    : m_db(db)
{
    if (!m_db)
    {
        throw std::runtime_error("Invalid database pointer");
    }
    // Соединение считается открытым сразу после создания.
    // В SQLite нет явного понятия "открыть соединение" — есть только
    // открытие БД.
    m_isOpen = true;
}

SqliteConnection::~SqliteConnection()
{
    try
    {
        close();
    }
    catch (...)
    {
        // Игнорируем исключения в деструкторе.
        // TODO: Добавить лог.
    }
}

void SqliteConnection::open(const std::string& /*connectionString*/)
{
    // Для SQLite этот метод является заглушкой, потому что:
    // 1) База данных уже открыта в SqliteDatabase::initialize().
    // 2) Строка подключения уже была использована там же.
    // Метод оставлен для соблюдения интерфейса IConnection.

    m_isOpen = true;
}

void SqliteConnection::close()
{
    // "Закрытие соединения" для SQLite означает лишь сброс флага.
    // Реальное закрытие БД происходит в SqliteDatabase::shutdown().
    m_isOpen = false;
}

bool SqliteConnection::isOpen() const
{
    return m_isOpen;
}

std::unique_ptr<IStatement> SqliteConnection::prepareStatement(
    const std::string& sql
)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    sqlite3* handle = m_db->getHandle();
    sqlite3_stmt* stmt = nullptr;
    const char* tail = nullptr;

    // Подготавливаем параметризованный запрос.
    // SQLite сам разберёт SQL и выделит места под параметры (:name, ? и т.д.).
    const int rc = sqlite3_prepare_v2(
        handle,
        sql.c_str(),
        static_cast<int>(sql.size()),
        &stmt,
        &tail
    );

    if (rc != SQLITE_OK)
    {
        throw std::runtime_error(
            "Failed to prepare statement: " + std::string(sqlite3_errmsg(handle))
        );
    }

    // SqliteStatement будет владеть stmt и удалит его в деструкторе.
    return std::make_unique<SqliteStatement>(handle, stmt);
}

int64_t SqliteConnection::execute(const std::string& sql)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }
    // Делегируем выполнение базовому объекту БД.
    return m_db->execute(sql);
}

std::unique_ptr<IResultSet> SqliteConnection::executeQuery(
    const std::string& sql
)
{
    // Вспомогательный метод для удобства: сразу выполняем SELECT.
    return m_db->query(sql);
}

std::unique_ptr<ITransaction> SqliteConnection::beginTransaction()
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    // Начинаем транзакцию через SQL-команду.
    execute("BEGIN TRANSACTION");
    // Возвращаем объект-обёртку, который будет управлять транзакцией.
    return std::make_unique<SqliteTransaction>(this);
}

int64_t SqliteConnection::getLastInsertId()
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }
    // sqlite3_last_insert_rowid — потокобезопасная функция,
    // возвращает ROWID последней вставки в рамках текущего соединения.
    return sqlite3_last_insert_rowid(m_db->getHandle());
}

std::string SqliteConnection::escapeString(const std::string& value)
{
    if (!m_isOpen)
    {
        throw std::runtime_error("Connection is not open");
    }

    // sqlite3_mprintf("%q") — экранирует строку для безопасного встраивания в SQL.
    // Это лучше, чем ручное экранирование, так как учитывает особенности SQLite.
    char* escaped = sqlite3_mprintf("%q", value.c_str());
    const std::string result(escaped);
    sqlite3_free(escaped); // Важно: sqlite3_mprintf использует свою кучу.
    return result;
}

} // namespace db::sqlite
