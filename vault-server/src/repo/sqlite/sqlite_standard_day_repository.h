#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../standard_day_repository.h"

namespace db
{
class IDatabase;
class IConnection;
class IResultSet;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite-реализация репозитория для работы со стандартными днями.
 */
class SqliteStandardDayRepository final : public IStandardDayRepository
{
public:
    explicit SqliteStandardDayRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteStandardDayRepository() override = default;

    SqliteStandardDayRepository(const SqliteStandardDayRepository&) = delete;
    SqliteStandardDayRepository& operator=(const SqliteStandardDayRepository&) = delete;

    std::vector<dto::StandardDay> findAll() override;
    std::optional<dto::StandardDay> findByWeekDayNumber(int weekDayNumber) override;
    bool update(const dto::StandardDay& standardDay) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::StandardDay mapRowToStandardDay(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
