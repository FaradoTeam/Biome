#include <regex>

#include <cpprest/uri.h>

#include "common/dto/team_message.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "team_messages_handler.h"

namespace server::handlers
{

TeamMessagesHandler::TeamMessagesHandler(
    std::shared_ptr<services::ITeamMessageService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "TeamMessagesHandler инициализирован без сервиса";
    }
}

// ============================================================
// GET /team-messages
// ============================================================

void TeamMessagesHandler::handleGetMessages(
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
    std::optional<int64_t> teamId = std::nullopt;
    if (params.count("teamId"))
    {
        try
        {
            teamId = std::stoll(params["teamId"]);
            if (teamId <= 0)
                teamId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetMessages: неверный параметр teamId: " << params["teamId"];
        }
    }

    std::optional<int64_t> senderUserId = std::nullopt;
    if (params.count("senderUserId"))
    {
        try
        {
            senderUserId = std::stoll(params["senderUserId"]);
            if (senderUserId <= 0)
                senderUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetMessages: неверный параметр senderUserId: " << params["senderUserId"];
        }
    }

    LOG_DEBUG
        << "GET /team-messages: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", teamId=" << (teamId.has_value() ? std::to_string(*teamId) : "none")
        << ", senderUserId=" << (senderUserId.has_value() ? std::to_string(*senderUserId) : "none");

    try
    {
        auto pageData = m_service->getMessages(
            page, pageSize, userId, teamId, senderUserId
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
        LOG_ERROR << "Ошибка при получении списка сообщений в командах: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /team-messages/{id}
// ============================================================

void TeamMessagesHandler::handleGetMessage(
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

    LOG_DEBUG << "GET /team-messages/" << messageId << " from user " << userId;

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
// GET /teams/{teamId}/messages
// ============================================================

void TeamMessagesHandler::handleGetTeamMessages(
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

    // Извлекаем teamId из пути: /teams/{teamId}/messages
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/teams/(\d+)/messages)");
    std::smatch matches;

    int64_t teamId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            teamId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid team ID");
            return;
        }
    }

    if (teamId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid team ID");
        return;
    }

    LOG_DEBUG << "GET /teams/" << teamId << "/messages from user " << userId;

    try
    {
        auto messages = m_service->getTeamMessages(teamId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < messages.size(); ++i)
        {
            response[i] = dto::toWebJson(messages[i].toJson());
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении сообщений команды: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /team-messages
// ============================================================

void TeamMessagesHandler::handleSendMessage(
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

    LOG_DEBUG << "POST /team-messages from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::TeamMessage message(nlohmannJson);

                    // Устанавливаем отправителя
                    message.senderUserId = userId;

                    // Валидация обязательных полей
                    if (!message.teamId.has_value() || *message.teamId <= 0)
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "teamId is required");
                        return;
                    }

                    if (!message.content.has_value() || message.content->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "content is required");
                        return;
                    }

                    auto created = m_service->sendMessage(message, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot send message: invalid data or not a team member"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " отправил сообщение в команду " << *message.teamId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при отправке сообщения в команду: " << e.what();
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
// DELETE /team-messages/{id}
// ============================================================

void TeamMessagesHandler::handleDeleteMessage(
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

    LOG_DEBUG << "DELETE /team-messages/" << messageId << " from user " << userId;

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

} // namespace server::handlers
