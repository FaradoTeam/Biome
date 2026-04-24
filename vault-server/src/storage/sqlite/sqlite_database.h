#pragma once

#include <atomic>
#include <mutex>
#include <memory>
#include <string>

#include "storage/idatabase.h"

namespace db
{

/**
 * @brief Реализация IDatabase для SQLite.
 *
 * @details Предоставляет доступ к SQLite БД через единственное активное
 *          соединение, которое автоматически пересоздается при необходимости.
 *
 * Реализация использует weak_ptr для отслеживания времени жизни соединения:
 * - Если соединение живо — возвращает его всем запрашивающим
 * - Если все клиенты освободили соединение — оно автоматически закрывается
 * - При следующем запросе создается новое соединение
 *
 * Для поддержки многопоточности:
 * - Включается WAL режим (параллельное чтение)
 * - Устанавливается busy_timeout (ожидание вместо мгновенной ошибки)
 * - Потокобезопасность на уровне соединения (shared_mutex)
 *
 * @note В отличие от других СУБД, SQLite оптимизирован для одного соединения
 *       на базу данных в пределах процесса. Данная реализация следует этой
 *       рекомендации, переиспользуя одно активное соединение.
 */
class SqliteDatabase final : public IDatabase
{
public:
    SqliteDatabase();
    ~SqliteDatabase() override;

    // Запрещаем копирование и перемещение, чтобы избежать дублирования соединений
    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;
    SqliteDatabase(SqliteDatabase&&) = delete;
    SqliteDatabase& operator=(SqliteDatabase&&) = delete;

    // Реализация IDatabase
    void initialize(const DatabaseConfig& config) override;
    void shutdown() override;
    std::shared_ptr<IConnection> connection() override;

private:
    /// Путь к файлу БД
    std::string m_dbPath;

    /// Флаг инициализации
    std::atomic<bool> m_initialized;

    /// Слабая ссылка на активное соединение
    std::weak_ptr<IConnection> m_activeConnection;

    std::mutex m_connectionMutex;
};

} // namespace db
