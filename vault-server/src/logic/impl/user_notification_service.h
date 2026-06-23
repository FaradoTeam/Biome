#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_notification_service.h"

#include "repo/item_repository.h"
#include "repo/user_notification_repository.h"
#include "repo/user_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для работы с подписками на уведомления.
 */
class UserNotificationService final : public IUserNotificationService
{
public:
    UserNotificationService(
        std::shared_ptr<repositories::IUserNotificationRepository> notificationRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<repositories::IItemRepository> itemRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserNotificationService
    UserNotificationsPage getNotifications(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<int64_t> itemId = std::nullopt
    ) override;

    std::optional<dto::UserNotification> getNotification(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::UserNotification> getUserNotifications(
        int64_t userId
    ) override;

    std::vector<int64_t> getSubscriberIds(
        int64_t itemId,
        int64_t userId
    ) override;

    bool isSubscribed(int64_t userId, int64_t itemId) override;

    std::optional<dto::UserNotification> subscribe(
        const dto::UserNotification& notification,
        int64_t userId
    ) override;

    UserNotificationResult unsubscribe(
        int64_t id,
        int64_t userId
    ) override;

    UserNotificationResult unsubscribeByUserAndItem(
        int64_t userId,
        int64_t itemId
    ) override;

private:
    /**
     * @brief Проверяет существование пользователя.
     * @param userId ID пользователя
     * @return true если пользователь существует
     */
    bool userExists(int64_t userId);

    /**
     * @brief Проверяет существование элемента.
     * @param itemId ID элемента
     * @return true если элемент существует
     */
    bool itemExists(int64_t itemId);

private:
    std::shared_ptr<repositories::IUserNotificationRepository> m_notificationRepo;
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<repositories::IItemRepository> m_itemRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
