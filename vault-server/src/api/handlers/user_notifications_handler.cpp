#include <regex>

#include <cpprest/uri.h>

#include "common/dto/user_notification.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_notifications_handler.h"

namespace server::handlers
{

UserNotificationsHandler::UserNotificationsHandler(
    std::shared_ptr<services::IUserNotificationService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "UserNotificationsHandler инициализирован без сервиса";
    }
}

// ============================================================
// GET /user-notifications
// ============================================================

void UserNotificationsHandler::handleGetNotifications(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    auto params = extractQueryParams(request);

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetNotifications: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetNotifications: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> filterUserId = std::nullopt;
    if (params.count("userId"))
    {
        try
        {
            filterUserId = std::stoll(params["userId"]);
            if (filterUserId <= 0)
                filterUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetNotifications: неверный параметр userId: " << params["userId"];
        }
    }

    std::optional<int64_t> itemId = std::nullopt;
    if (params.count("itemId"))
    {
        try
        {
            itemId = std::stoll(params["itemId"]);
            if (itemId <= 0)
                itemId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetNotifications: неверный параметр itemId: " << params["itemId"];
        }
    }

    LOG_DEBUG
        << "GET /user-notifications: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none")
        << ", itemId=" << (itemId.has_value() ? std::to_string(*itemId) : "none");

    try
    {
        auto pageData = m_service->getNotifications(
            page, pageSize, userId, filterUserId, itemId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.notifications.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.notifications[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка подписок: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /user-notifications/{id}
// ============================================================

void UserNotificationsHandler::handleGetNotification(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t notificationId = extractIdFromPath(request);
    if (notificationId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid notification ID");
        return;
    }

    LOG_DEBUG << "GET /user-notifications/" << notificationId << " from user " << userId;

    try
    {
        auto notification = m_service->getNotification(notificationId, userId);
        if (!notification)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Notification not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(notification->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении подписки " << notificationId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /user-notifications
// ============================================================

void UserNotificationsHandler::handleSubscribe(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /user-notifications from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::UserNotification notification(nlohmannJson);

                    // Валидация обязательных полей
                    if (!notification.itemId.has_value() || *notification.itemId <= 0)
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "itemId is required");
                        return;
                    }

                    // Если userId не указан, подписываем текущего пользователя
                    if (!notification.userId.has_value())
                    {
                        notification.userId = userId;
                    }

                    auto created = m_service->subscribe(notification, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Conflict,
                            "Already subscribed or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " подписался на элемент " << *notification.itemId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании подписки: " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /user-notifications/{id}
// ============================================================

void UserNotificationsHandler::handleUnsubscribe(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t notificationId = extractIdFromPath(request);
    if (notificationId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid notification ID");
        return;
    }

    LOG_DEBUG << "DELETE /user-notifications/" << notificationId << " from user " << userId;

    try
    {
        auto result = m_service->unsubscribe(notificationId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO << "Пользователь " << userId << " отписался от элемента";

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при отписке: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /items/{itemId}/subscribers
// ============================================================

void UserNotificationsHandler::handleGetSubscribers(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем itemId из пути: /items/{itemId}/subscribers
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/subscribers)");
    std::smatch matches;

    int64_t itemId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/subscribers from user " << userId;

    try
    {
        auto subscriberIds = m_service->getSubscriberIds(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < subscriberIds.size(); ++i)
        {
            response[i] = web::json::value::number(subscriberIds[i]);
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении подписчиков элемента: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /items/{itemId}/subscribed
// ============================================================

void UserNotificationsHandler::handleIsSubscribed(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем itemId из пути: /items/{itemId}/subscribed
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/subscribed)");
    std::smatch matches;

    int64_t itemId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/subscribed from user " << userId;

    try
    {
        bool isSubscribed = m_service->isSubscribed(userId, itemId);

        web::json::value response;
        response[U("subscribed")] = web::json::value::boolean(isSubscribed);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при проверке подписки: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
