#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "base_handler.h"
#include "logic/iuser_notification_service.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы с подписками на уведомления.
 */
class UserNotificationsHandler final : public BaseHandler
{
public:
    explicit UserNotificationsHandler(
        std::shared_ptr<services::IUserNotificationService> service
    );

    /**
     * @brief Получает список подписок с пагинацией и фильтрацией.
     * GET /user-notifications
     */
    void handleGetNotifications(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает подписку по ID.
     * GET /user-notifications/{id}
     */
    void handleGetNotification(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Подписывает пользователя на элемент.
     * POST /user-notifications
     */
    void handleSubscribe(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Отписывает пользователя от элемента.
     * DELETE /user-notifications/{id}
     */
    void handleUnsubscribe(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает ID всех подписчиков элемента.
     * GET /items/{itemId}/subscribers
     */
    void handleGetSubscribers(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Проверяет, подписан ли пользователь на элемент.
     * GET /items/{itemId}/subscribed
     */
    void handleIsSubscribed(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IUserNotificationService> m_service;
};

} // namespace server::handlers
