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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка личных сообщений: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid message ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /private-messages/" << messageId << " from user " << userId;

    try
    {
        auto message = m_service->getMessage(messageId, userId);
        if (!message)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Message not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(message->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении сообщения " << messageId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid user ID");
            request.reply(resp);
            return;
        }
    }

    if (otherUserId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
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

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении переписки: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "receiverUserId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!message.content.has_value() || message.content->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "content is required");
                        request.reply(resp);
                        return;
                    }

                    // Нельзя отправить сообщение самому себе
                    if (*message.receiverUserId == userId)
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Cannot send message to yourself");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->sendMessage(message, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot send message: invalid data or recipient not found"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " отправил сообщение пользователю " << *message.receiverUserId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при отправке сообщения: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid message ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /private-messages/" << messageId << "/view from user " << userId;

    try
    {
        auto result = m_service->markAsViewed(messageId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO << "Пользователь " << userId << " отметил сообщение " << messageId << " как прочитанное";

        request.reply(web::http::status_codes::OK);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при отметке сообщения как прочитанного: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t messageId = extractIdFromPath(request);
    if (messageId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid message ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /private-messages/" << messageId << " from user " << userId;

    try
    {
        auto result = m_service->deleteMessage(messageId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил сообщение " << messageId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении сообщения " << messageId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "GET /private-messages/unviewed/count from user " << userId;

    try
    {
        int64_t count = m_service->countUnviewed(userId);

        web::json::value response;
        response["count"] = web::json::value::number(count);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при подсчёте непрочитанных сообщений: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace server::handlers
