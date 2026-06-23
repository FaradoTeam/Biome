#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/user_notification.h"

namespace server::services
{

/**
 * @brief Страница с подписками на уведомления.
 */
struct UserNotificationsPage
{
    std::vector<dto::UserNotification> notifications;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с подпиской на уведомления.
 */
struct UserNotificationResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с подписками на уведомления.
 */
class IUserNotificationService
{
public:
    virtual ~IUserNotificationService() = default;

    /**
     * @brief Получает список подписок с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по пользователю (std::nullopt - все)
     * @param itemId Фильтр по элементу (std::nullopt - все)
     * @return Страница с подписками
     */
    virtual UserNotificationsPage getNotifications(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<int64_t> itemId = std::nullopt
    ) = 0;

    /**
     * @brief Получает подписку по ID.
     * @param id Идентификатор подписки
     * @param userId ID пользователя для проверки прав
     * @return DTO подписки или std::nullopt
     */
    virtual std::optional<dto::UserNotification> getNotification(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все подписки пользователя.
     * @param userId ID пользователя
     * @return Вектор подписок
     */
    virtual std::vector<dto::UserNotification> getUserNotifications(
        int64_t userId
    ) = 0;

    /**
     * @brief Получает ID всех подписчиков элемента.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав на чтение элемента
     * @return Вектор ID пользователей
     */
    virtual std::vector<int64_t> getSubscriberIds(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Проверяет, подписан ли пользователь на элемент.
     * @param userId ID пользователя
     * @param itemId Идентификатор элемента
     * @return true если подписан
     */
    virtual bool isSubscribed(int64_t userId, int64_t itemId) = 0;

    /**
     * @brief Подписывает пользователя на элемент.
     * @param notification DTO подписки (userId и itemId)
     * @param userId ID пользователя для проверки прав
     * @return Созданная подписка или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserNotification> subscribe(
        const dto::UserNotification& notification,
        int64_t userId
    ) = 0;

    /**
     * @brief Отписывает пользователя от элемента по ID подписки.
     * @param id Идентификатор подписки
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual UserNotificationResult unsubscribe(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Отписывает пользователя от элемента по паре (userId, itemId).
     * @param userId ID пользователя
     * @param itemId Идентификатор элемента
     * @return Результат операции
     */
    virtual UserNotificationResult unsubscribeByUserAndItem(
        int64_t userId,
        int64_t itemId
    ) = 0;
};

} // namespace server::services
