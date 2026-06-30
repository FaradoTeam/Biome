#pragma once

#include <memory>

#include "../user_action_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с действиями пользователя.
 */
class SqliteUserActionRepository final : public IUserActionRepository
{
public:
    explicit SqliteUserActionRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserActionRepository() override = default;

    SqliteUserActionRepository(const SqliteUserActionRepository&) = delete;
    SqliteUserActionRepository& operator=(const SqliteUserActionRepository&) = delete;

    // IUserActionRepository
    UserActionsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::UserAction> findById(int64_t id) override;
    std::vector<dto::UserAction> findByUserId(int64_t userId) override;
    int64_t create(const dto::UserAction& action) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::UserAction mapRowToUserAction(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
