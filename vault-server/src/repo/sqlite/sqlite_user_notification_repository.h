#pragma once

#include <memory>

#include "../user_notification_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

class SqliteUserNotificationRepository final : public IUserNotificationRepository
{
public:
    explicit SqliteUserNotificationRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserNotificationRepository() override = default;

    UserNotificationsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> itemId = std::nullopt
    ) override;

    std::optional<dto::UserNotification> findById(int64_t id) override;
    std::optional<dto::UserNotification> findByUserAndItem(
        int64_t userId,
        int64_t itemId
    ) override;
    std::vector<dto::UserNotification> findByUserId(int64_t userId) override;
    std::vector<dto::UserNotification> findByItemId(int64_t itemId) override;
    bool exists(int64_t userId, int64_t itemId) override;
    int64_t create(const dto::UserNotification& notification) override;
    bool remove(int64_t id) override;
    bool removeByUserAndItem(int64_t userId, int64_t itemId) override;
    int64_t removeByUserId(int64_t userId) override;
    int64_t removeByItemId(int64_t itemId) override;
    std::vector<int64_t> getSubscriberIds(int64_t itemId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
