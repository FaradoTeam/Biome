#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../user_day_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с пользовательскими днями.
 */
class SqliteUserDayRepository final : public IUserDayRepository
{
public:
    explicit SqliteUserDayRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserDayRepository() override = default;

    SqliteUserDayRepository(const SqliteUserDayRepository&) = delete;
    SqliteUserDayRepository& operator=(const SqliteUserDayRepository&) = delete;

    UserDaysPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::UserDay> findById(int64_t id) override;
    std::optional<dto::UserDay> findByUserAndDate(
        int64_t userId,
        const common::DateTime& date
    ) override;
    std::vector<dto::UserDay> findByUserId(int64_t userId) override;
    int64_t create(const dto::UserDay& userDay) override;
    bool update(const dto::UserDay& userDay) override;
    bool remove(int64_t id) override;
    int64_t removeByUserId(int64_t userId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::UserDay mapRowToUserDay(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
