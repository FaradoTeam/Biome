#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

#include "storage/idatabase.h"

#include "sqlite_result_set.h"

struct sqlite3;

namespace db
{

class SqliteStatement;

/**
 * @brief Реализация соединения с SQLite базой данных.
 */
class SqliteConnection final : public IConnection
{
public:
    /**
     * @brief RAII-обёртка для блокировки соединения.
     *
     * Если текущий поток уже находится в транзакции (isInTransaction == true),
     * блокировка не захватывается, так как транзакция уже эксклюзивно владеет
     * мьютексом.
     *
     * @tparam LockType Тип блокировки (std::unique_lock или std::shared_lock)
     */
    template <typename LockType>
    class [[nodiscard]] ScopedLock
    {
    public:
        explicit ScopedLock(const SqliteConnection& connection)
            : m_shouldLock(!connection.isInTransaction)
        {
            if (m_shouldLock)
            {
                m_lock.emplace(connection.m_mutex);
            }
        }

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
        ScopedLock(ScopedLock&&) = delete;
        ScopedLock& operator=(ScopedLock&&) = delete;

    private:
        bool m_shouldLock;
        std::optional<LockType> m_lock;
    };
    using ExclusiveLock = ScopedLock<std::unique_lock<std::shared_mutex>>;
    using SharedLock = ScopedLock<std::shared_lock<std::shared_mutex>>;

public:
    /**
     * @brief Конструктор. Открывает соединение с SQLite БД.
     * @param dbPath Путь к файлу базы данных SQLite
     * @throws std::runtime_error Если не удалось открыть БД
     *
     * @details Если файл не существует, SQLite создаст его автоматически.
     *          Путь нормализуется (удаляются лишние слеши, создаются директории).
     */
    explicit SqliteConnection(const std::string& dbPath);

    /// Деструктор. Закрывает соединение с БД.
    ~SqliteConnection() override;

    // Запрещаем копирование и перемещение, так как соединение
    // управляет уникальным ресурсом (sqlite3*)
    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;
    SqliteConnection(SqliteConnection&&) = delete;
    SqliteConnection& operator=(SqliteConnection&&) = delete;

    // Реализация методов IConnection
    std::unique_ptr<IStatement> prepareStatement(const std::string& sql) override;
    int64_t execute(const std::string& sql) override;
    std::unique_ptr<ITransaction> beginTransaction() override;
    int64_t lastInsertId() override;
    std::string escapeString(const std::string& value) override;

private:
    // Дружественные классы для доступа к внутренностям соединения
    friend class SqliteStatement;
    friend class SqliteTransaction;
    friend class SqliteResultSet;

private:
    /// thread_local флаг: этот поток внутри транзакции?
    inline static thread_local bool isInTransaction = false;

private:
    /**
     * @brief Проверяет код ошибки SQLite и бросает исключение при необходимости.
     * @param rc Код возврата SQLite
     * @param operation Название операции для сообщения об ошибке
     * @throws std::runtime_error Если rc указывает на ошибку
     */
    void checkError(int rc, const std::string& operation) const;

    /// Возвращает указатель на внутренний объект sqlite3
    sqlite3* handle() const { return m_db; }

    /// Возвращает мьютекс для синхронизации доступа
    std::shared_mutex& mutex() { return m_mutex; }

private:
    sqlite3* m_db = { nullptr }; ///< Указатель на объект SQLite3
    mutable std::shared_mutex m_mutex; ///< Мьютекс для потокобезопасности

};

} // namespace db
