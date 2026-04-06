#pragma once
#include <functional>
#include <memory>
#include <string>

#include "types.h"

namespace db
{

/**
 * @brief Главный интерфейс базы данных.
 *
 * Предоставляет методы для инициализации, выполнения запросов,
 * управления транзакциями и получения соединений.
 */
class IConnection;
class IStatement;
class IResultSet;
class ITransaction;

class IDatabase
{
public:
    virtual ~IDatabase() = default;

    /**
     * @brief Инициализирует подключение к базе данных.
     * @param config Конфигурация подключения (например, путь к файлу для SQLite)
     */
    virtual void initialize(const DatabaseConfig& config) = 0;

    /**
     * @brief Закрывает соединение и освобождает ресурсы.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Возвращает новое соединение с базой данных.
     * @return unique_ptr к объекту IConnection
     */
    virtual std::unique_ptr<IConnection> getConnection() = 0;

    /**
     * @brief Выполняет SQL-запрос и возвращает набор результатов.
     * @param sql SQL-запрос (обычно SELECT)
     * @return Результат запроса в виде IResultSet
     */
    virtual std::unique_ptr<IResultSet> query(const std::string& sql) = 0;

    /**
     * @brief Выполняет SQL-команду (INSERT, UPDATE, DELETE и т.д.).
     * @param sql SQL-команда
     * @return Количество затронутых строк
     */
    virtual int64_t execute(const std::string& sql) = 0;

    /**
     * @brief Выполняет callback в рамках транзакции.
     * @param callback Функция, выполняемая внутри транзакции
     * @note При возникновении исключения выполняется ROLLBACK
     */
    virtual void transaction(std::function<void()> callback) = 0;
};

//-----------------------------------------------------------------------------

/**
 * @brief Интерфейс соединения с базой данных.
 *
 * Предоставляет методы для управления соединением, подготовки
 * параметризованных запросов и управления транзакциями.
 */
class IConnection
{
public:
    virtual ~IConnection() = default;

    /// Открывает соединение (повторно использует строку подключения)
    virtual void open(const std::string& connectionString) = 0;
    /// Закрывает соединение
    virtual void close() = 0;
    /// Проверяет, активно ли соединение
    virtual bool isOpen() const = 0;

    /**
     * @brief Подготавливает параметризованный SQL-запрос.
     * @param sql SQL с именованными параметрами (например, :name)
     * @return Подготовленный запрос
     */
    virtual std::unique_ptr<IStatement> prepareStatement(const std::string& sql) = 0;

    /**
     * @brief Выполняет SQL-команду напрямую.
     * @param sql SQL-команда
     * @return Количество затронутых строк
     */
    virtual int64_t execute(const std::string& sql) = 0;

    /**
     * @brief Начинает новую транзакцию.
     * @return Объект транзакции (commit/rollback в деструкторе не делается)
     */
    virtual std::unique_ptr<ITransaction> beginTransaction() = 0;

    /// Возвращает ID последней вставленной записи
    virtual int64_t getLastInsertId() = 0;

    /// Экранирует спецсимволы в строке для безопасного встраивания в SQL
    virtual std::string escapeString(const std::string& value) = 0;
};

//-----------------------------------------------------------------------------

/**
 * @brief Интерфейс параметризованного SQL-запроса (prepared statement).
 *
 * Позволяет связывать значения с именованными параметрами и выполнять запрос.
 */
class IStatement
{
public:
    virtual ~IStatement() = default;

    /**
     * @brief Выполняет запрос, который не возвращает строк (INSERT/UPDATE/DELETE).
     * @return Количество затронутых строк
     * @throws Если запрос возвращает строки (нужен executeQuery)
     */
    virtual int64_t execute() = 0;

    /**
     * @brief Выполняет SELECT-запрос и возвращает результат.
     * @return Набор строк
     */
    virtual std::unique_ptr<IResultSet> executeQuery() = 0;

    /// Сбрасывает состояние запроса, очищает привязки
    virtual void reset() = 0;

    // --- Методы привязки значений к именованным параметрам ---
    virtual void bindNull(const std::string& name) = 0;
    virtual void bindInt64(const std::string& name, int64_t value) = 0;
    virtual void bindDouble(const std::string& name, double value) = 0;
    virtual void bindString(const std::string& name, const std::string& value) = 0;
    virtual void bindBlob(const std::string& name, const Blob& value) = 0;
    virtual void bindDateTime(const std::string& name, const DateTime& value) = 0;
};

//-----------------------------------------------------------------------------

/**
 * @brief Интерфейс набора результатов (результат SELECT-запроса).
 *
 * Позволяет итерироваться по строкам и получать значения колонок по индексу или имени.
 */
class IResultSet
{
public:
    virtual ~IResultSet() = default;

    /**
     * @brief Перемещает курсор на следующую строку.
     * @return true - строка существует, false - строк больше нет
     */
    virtual bool next() = 0;

    /// Возвращает количество колонок в результате
    virtual int getColumnCount() const = 0;
    /// Возвращает имя колонки по индексу
    virtual std::string getColumnName(int index) const = 0;
    /// Возвращает индекс колонки по имени
    virtual int getColumnIndex(const std::string& name) const = 0;

    /// Проверяет, является ли значение в колонке NULL
    virtual bool isNull(int index) const = 0;
    virtual bool isNull(const std::string& name) const
    {
        return isNull(getColumnIndex(name));
    }

    /// Возвращает значение как универсальный FieldValue
    virtual FieldValue getValue(int index) const = 0;
    virtual FieldValue getValue(const std::string& name) const
    {
        return getValue(getColumnIndex(name));
    }

    // --- Типизированные геттеры (по индексу) ---
    virtual int64_t getInt64(int index) const
    {
        return getValue(index).asInt64();
    }
    virtual double getDouble(int index) const
    {
        return getValue(index).asDouble();
    }
    virtual std::string getString(int index) const
    {
        return getValue(index).asString();
    }
    virtual Blob getBlob(int index) const
    {
        return getValue(index).asBlob();
    }
    virtual DateTime getDateTime(int index) const
    {
        return getValue(index).asDateTime();
    }

    // --- Типизированные геттеры (по имени) ---
    virtual int64_t getInt64(const std::string& name) const
    {
        return getInt64(getColumnIndex(name));
    }
    virtual double getDouble(const std::string& name) const
    {
        return getDouble(getColumnIndex(name));
    }
    virtual std::string getString(const std::string& name) const
    {
        return getString(getColumnIndex(name));
    }
    virtual Blob getBlob(const std::string& name) const
    {
        return getBlob(getColumnIndex(name));
    }
    virtual DateTime getDateTime(const std::string& name) const
    {
        return getDateTime(getColumnIndex(name));
    }
};

//-----------------------------------------------------------------------------

/**
 * @brief Интерфейс транзакции.
 *
 * Позволяет явно зафиксировать или откатить изменения.
 * Транзакция создаётся через IConnection::beginTransaction().
 */
class ITransaction
{
public:
    virtual ~ITransaction() = default;

    /// Фиксирует все изменения в транзакции
    virtual void commit() = 0;
    /// Отменяет все изменения в транзакции
    virtual void rollback() = 0;
    /// Активна ли транзакция (не была завершена)
    virtual bool isActive() const = 0;
};

} // namespace db
