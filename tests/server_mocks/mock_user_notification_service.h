#pragma once

#include <optional>
#include <vector>

#include "logic/iuser_notification_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для IUserNotificationService.
 */
class MockUserNotificationService : public services::IUserNotificationService
{
public:
    // ============================================================
    // Создание тестовых DTO
    // ============================================================

    static dto::UserNotification createTestNotification(
        int64_t id,
        int64_t userId,
        int64_t itemId
    )
    {
        dto::UserNotification notif;
        notif.id = id;
        notif.userId = userId;
        notif.itemId = itemId;
        return notif;
    }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        m_getNotificationsCallCount = 0;
        m_getNotificationCallCount = 0;
        m_getUserNotificationsCallCount = 0;
        m_getSubscriberIdsCallCount = 0;
        m_isSubscribedCallCount = 0;
        m_subscribeCallCount = 0;
        m_unsubscribeCallCount = 0;
        m_unsubscribeByUserAndItemCallCount = 0;

        m_lastGetNotificationsUserId = 0;
        m_lastGetNotificationsPage = 0;
        m_lastGetNotificationsPageSize = 0;
        m_lastGetNotificationsFilterUserId = std::nullopt;
        m_lastGetNotificationsItemId = std::nullopt;

        m_lastGetNotificationId = 0;
        m_lastGetUserNotificationsUserId = 0;
        m_lastGetSubscriberIdsItemId = 0;
        m_lastGetSubscriberIdsUserId = 0;
        m_lastIsSubscribedUserId = 0;
        m_lastIsSubscribedItemId = 0;
        m_lastSubscribeUserId = 0;
        m_lastUnsubscribeId = 0;
        m_lastUnsubscribeUserId = 0;
        m_lastUnsubscribeByUserAndItemUserId = 0;
        m_lastUnsubscribeByUserAndItemItemId = 0;

        m_getNotificationsResult = services::UserNotificationsPage { {}, 0 };
        m_getNotificationResult = std::nullopt;
        m_getUserNotificationsResult = {};
        m_getSubscriberIdsResult = {};
        m_isSubscribedResult = false;
        m_subscribeResult = std::nullopt;
        m_unsubscribeResult = services::UserNotificationResult { true, 0, "" };
    }

    MockUserNotificationService()
    {
        reset();
    }

    // ============================================================
    // Настройка возвращаемых значений
    // ============================================================

    void setGetNotificationsResult(const services::UserNotificationsPage& result)
    {
        m_getNotificationsResult = result;
    }

    void setGetNotificationResult(const std::optional<dto::UserNotification>& result)
    {
        m_getNotificationResult = result;
    }

    void setGetUserNotificationsResult(const std::vector<dto::UserNotification>& result)
    {
        m_getUserNotificationsResult = result;
    }

    void setGetSubscriberIdsResult(const std::vector<int64_t>& result)
    {
        m_getSubscriberIdsResult = result;
    }

    void setIsSubscribedResult(bool result)
    {
        m_isSubscribedResult = result;
    }

    void setSubscribeResult(const std::optional<dto::UserNotification>& result)
    {
        m_subscribeResult = result;
    }

    void setUnsubscribeResult(const services::UserNotificationResult& result)
    {
        m_unsubscribeResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetNotificationsCallCount() const { return m_getNotificationsCallCount; }
    int getGetNotificationCallCount() const { return m_getNotificationCallCount; }
    int getGetUserNotificationsCallCount() const { return m_getUserNotificationsCallCount; }
    int getGetSubscriberIdsCallCount() const { return m_getSubscriberIdsCallCount; }
    int getIsSubscribedCallCount() const { return m_isSubscribedCallCount; }
    int getSubscribeCallCount() const { return m_subscribeCallCount; }
    int getUnsubscribeCallCount() const { return m_unsubscribeCallCount; }
    int getUnsubscribeByUserAndItemCallCount() const { return m_unsubscribeByUserAndItemCallCount; }

    int64_t getLastGetNotificationsUserId() const { return m_lastGetNotificationsUserId; }
    int getLastGetNotificationsPage() const { return m_lastGetNotificationsPage; }
    int getLastGetNotificationsPageSize() const { return m_lastGetNotificationsPageSize; }
    std::optional<int64_t> getLastGetNotificationsFilterUserId() const { return m_lastGetNotificationsFilterUserId; }
    std::optional<int64_t> getLastGetNotificationsItemId() const { return m_lastGetNotificationsItemId; }

    int64_t getLastGetNotificationId() const { return m_lastGetNotificationId; }
    int64_t getLastGetUserNotificationsUserId() const { return m_lastGetUserNotificationsUserId; }
    int64_t getLastGetSubscriberIdsItemId() const { return m_lastGetSubscriberIdsItemId; }
    int64_t getLastGetSubscriberIdsUserId() const { return m_lastGetSubscriberIdsUserId; }
    int64_t getLastIsSubscribedUserId() const { return m_lastIsSubscribedUserId; }
    int64_t getLastIsSubscribedItemId() const { return m_lastIsSubscribedItemId; }
    int64_t getLastSubscribeUserId() const { return m_lastSubscribeUserId; }
    int64_t getLastUnsubscribeId() const { return m_lastUnsubscribeId; }
    int64_t getLastUnsubscribeUserId() const { return m_lastUnsubscribeUserId; }
    int64_t getLastUnsubscribeByUserAndItemUserId() const { return m_lastUnsubscribeByUserAndItemUserId; }
    int64_t getLastUnsubscribeByUserAndItemItemId() const { return m_lastUnsubscribeByUserAndItemItemId; }

    // ============================================================
    // Реализация интерфейса IUserNotificationService
    // ============================================================

    services::UserNotificationsPage getNotifications(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId,
        std::optional<int64_t> itemId
    ) override
    {
        m_getNotificationsCallCount++;
        m_lastGetNotificationsUserId = userId;
        m_lastGetNotificationsPage = page;
        m_lastGetNotificationsPageSize = pageSize;
        m_lastGetNotificationsFilterUserId = filterUserId;
        m_lastGetNotificationsItemId = itemId;
        return m_getNotificationsResult;
    }

    std::optional<dto::UserNotification> getNotification(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getNotificationCallCount++;
        m_lastGetNotificationId = id;
        m_lastGetNotificationUserId = userId;
        return m_getNotificationResult;
    }

    std::vector<dto::UserNotification> getUserNotifications(
        int64_t userId
    ) override
    {
        m_getUserNotificationsCallCount++;
        m_lastGetUserNotificationsUserId = userId;
        return m_getUserNotificationsResult;
    }

    std::vector<int64_t> getSubscriberIds(
        int64_t itemId,
        int64_t userId
    ) override
    {
        m_getSubscriberIdsCallCount++;
        m_lastGetSubscriberIdsItemId = itemId;
        m_lastGetSubscriberIdsUserId = userId;
        return m_getSubscriberIdsResult;
    }

    bool isSubscribed(
        int64_t userId,
        int64_t itemId
    ) override
    {
        m_isSubscribedCallCount++;
        m_lastIsSubscribedUserId = userId;
        m_lastIsSubscribedItemId = itemId;
        return m_isSubscribedResult;
    }

    std::optional<dto::UserNotification> subscribe(
        const dto::UserNotification& notification,
        int64_t userId
    ) override
    {
        m_subscribeCallCount++;
        m_lastSubscribeUserId = userId;
        return m_subscribeResult;
    }

    services::UserNotificationResult unsubscribe(
        int64_t id,
        int64_t userId
    ) override
    {
        m_unsubscribeCallCount++;
        m_lastUnsubscribeId = id;
        m_lastUnsubscribeUserId = userId;
        return m_unsubscribeResult;
    }

    services::UserNotificationResult unsubscribeByUserAndItem(
        int64_t userId,
        int64_t itemId
    ) override
    {
        m_unsubscribeByUserAndItemCallCount++;
        m_lastUnsubscribeByUserAndItemUserId = userId;
        m_lastUnsubscribeByUserAndItemItemId = itemId;
        return m_unsubscribeResult;
    }

private:
    // Счётчики вызовов
    int m_getNotificationsCallCount = 0;
    int m_getNotificationCallCount = 0;
    int m_getUserNotificationsCallCount = 0;
    int m_getSubscriberIdsCallCount = 0;
    int m_isSubscribedCallCount = 0;
    int m_subscribeCallCount = 0;
    int m_unsubscribeCallCount = 0;
    int m_unsubscribeByUserAndItemCallCount = 0;

    // Параметры последних вызовов
    int64_t m_lastGetNotificationsUserId = 0;
    int m_lastGetNotificationsPage = 0;
    int m_lastGetNotificationsPageSize = 0;
    std::optional<int64_t> m_lastGetNotificationsFilterUserId;
    std::optional<int64_t> m_lastGetNotificationsItemId;

    int64_t m_lastGetNotificationId = 0;
    int64_t m_lastGetNotificationUserId = 0;
    int64_t m_lastGetUserNotificationsUserId = 0;
    int64_t m_lastGetSubscriberIdsItemId = 0;
    int64_t m_lastGetSubscriberIdsUserId = 0;
    int64_t m_lastIsSubscribedUserId = 0;
    int64_t m_lastIsSubscribedItemId = 0;
    int64_t m_lastSubscribeUserId = 0;
    int64_t m_lastUnsubscribeId = 0;
    int64_t m_lastUnsubscribeUserId = 0;
    int64_t m_lastUnsubscribeByUserAndItemUserId = 0;
    int64_t m_lastUnsubscribeByUserAndItemItemId = 0;

    // Возвращаемые значения
    services::UserNotificationsPage m_getNotificationsResult;
    std::optional<dto::UserNotification> m_getNotificationResult;
    std::vector<dto::UserNotification> m_getUserNotificationsResult;
    std::vector<int64_t> m_getSubscriberIdsResult;
    bool m_isSubscribedResult = false;
    std::optional<dto::UserNotification> m_subscribeResult;
    services::UserNotificationResult m_unsubscribeResult;
};

} // namespace tests
} // namespace server
