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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка сообщений в командах: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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

    LOG_DEBUG << "GET /team-messages/" << messageId << " from user " << userId;

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
// GET /teams/{teamId}/messages
// ============================================================

void TeamMessagesHandler::handleGetTeamMessages(
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid team ID");
            request.reply(resp);
            return;
        }
    }

    if (teamId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid team ID");
        request.reply(resp);
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

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении сообщений команды: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "teamId is required");
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

                    auto created = m_service->sendMessage(message, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot send message: invalid data or not a team member"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " отправил сообщение в команду " << *message.teamId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при отправке сообщения в команду: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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

    LOG_DEBUG << "DELETE /team-messages/" << messageId << " from user " << userId;

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

} // namespace server::handlers
