#include <regex>

#include <cpprest/uri.h>

#include "common/dto/private_message.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "private_messages_handler.h"

namespace server::handlers
{

PrivateMessagesHandler::PrivateMessagesHandler(
    std::shared_ptr<services::IPrivateMessageService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "PrivateMessagesHandler инициализирован без сервиса";
    }
}

// ============================================================
// GET /private-messages
// ============================================================

void PrivateMessagesHandler::handleGetMessages(
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
            LOG_WARN << "handleGetMessages: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetMessages: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> filterUserId = std::nullopt;
    if (params.count("withUserId"))
    {
        try
        {
            filterUserId = std::stoll(params["withUserId"]);
            if (filterUserId <= 0)
                filterUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetMessages: неверный параметр withUserId: " << params["withUserId"];
        }
    }

    std::optional<bool> isViewed = std::nullopt;
    if (params.count("isViewed"))
    {
        isViewed = parseBool(params["isViewed"]);
    }

    LOG_DEBUG
        << "GET /private-messages: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", withUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none")
        << ", isViewed=" << (isViewed.has_value() ? (*isViewed ? "true" : "false") : "none");

    try
    {
        auto pageData = m_service->getMessages(
            page, pageSize, userId, filterUserId, isViewed
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.messages.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.messages[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка личных сообщений: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /private-messages/{id}
// ============================================================

void PrivateMessagesHandler::handleGetMessage(
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

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid message ID");
        return;
    }

    LOG_DEBUG << "GET /private-messages/" << messageId << " from user " << userId;

    try
    {
        auto message = m_service->getMessage(messageId, userId);
        if (!message)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Message not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(message->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении сообщения " << messageId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /private-messages/conversation/{userId}
// ============================================================

void PrivateMessagesHandler::handleGetConversation(
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

    // Извлекаем собеседника из пути: /private-messages/conversation/{otherUserId}
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/private-messages/conversation/(\d+))");
    std::smatch matches;

    int64_t otherUserId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            otherUserId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
            return;
        }
    }

    if (otherUserId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    LOG_DEBUG << "GET /private-messages/conversation/" << otherUserId << " from user " << userId;

    try
    {
        auto messages = m_service->getConversation(userId, otherUserId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < messages.size(); ++i)
        {
            response[i] = dto::toWebJson(messages[i].toJson());
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении переписки: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /private-messages
// ============================================================

void PrivateMessagesHandler::handleSendMessage(
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

    LOG_DEBUG << "POST /private-messages from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::PrivateMessage message(nlohmannJson);

                    // Устанавливаем отправителя
                    message.senderUserId = userId;

                    // Валидация обязательных полей
                    if (!message.receiverUserId.has_value() || *message.receiverUserId <= 0)
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "receiverUserId is required");
                        return;
                    }

                    if (!message.content.has_value() || message.content->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "content is required");
                        return;
                    }

                    // Нельзя отправить сообщение самому себе
                    if (*message.receiverUserId == userId)
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Cannot send message to yourself");
                        return;
                    }

                    auto created = m_service->sendMessage(message, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot send message: invalid data or recipient not found"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " отправил сообщение пользователю " << *message.receiverUserId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при отправке сообщения: " << e.what();
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
// PUT /private-messages/{id}/view
// ============================================================

void PrivateMessagesHandler::handleMarkAsViewed(
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

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid message ID");
        return;
    }

    LOG_DEBUG << "PUT /private-messages/" << messageId << "/view from user " << userId;

    try
    {
        auto result = m_service->markAsViewed(messageId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO << "Пользователь " << userId << " отметил сообщение " << messageId << " как прочитанное";

        web::http::http_response response(web::http::status_codes::OK);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при отметке сообщения как прочитанного: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// DELETE /private-messages/{id}
// ============================================================

void PrivateMessagesHandler::handleDeleteMessage(
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

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid message ID");
        return;
    }

    LOG_DEBUG << "DELETE /private-messages/" << messageId << " from user " << userId;

    try
    {
        auto result = m_service->deleteMessage(messageId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил сообщение " << messageId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении сообщения " << messageId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /private-messages/unviewed/count
// ============================================================

void PrivateMessagesHandler::handleCountUnviewed(
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

    LOG_DEBUG << "GET /private-messages/unviewed/count from user " << userId;

    try
    {
        int64_t count = m_service->countUnviewed(userId);

        web::json::value response;
        response["count"] = web::json::value::number(count);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при подсчёте непрочитанных сообщений: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
