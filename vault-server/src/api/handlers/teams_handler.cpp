#include <cpprest/uri.h>

#include "common/dto/team.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "teams_handler.h"

namespace server
{
namespace handlers
{

TeamsHandler::TeamsHandler(std::shared_ptr<services::ITeamService> teamService)
    : m_teamService(std::move(teamService))
{
    if (!m_teamService)
    {
        LOG_WARN << "TeamsHandler инициализирован без TeamService";
    }
}

void TeamsHandler::handleGetTeams(
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
            LOG_WARN << "handleGetTeams: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetTeams: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтр по названию
    std::string searchCaption;
    if (params.count("caption"))
    {
        searchCaption = params["caption"];
    }

    LOG_DEBUG
        << "GET /teams: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", searchCaption=" << searchCaption;

    try
    {
        auto teamsPage = m_teamService->getTeams(page, pageSize, userId, searchCaption);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < teamsPage.teams.size(); ++i)
        {
            items[i] = dto::toWebJson(teamsPage.teams[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(teamsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка команд: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void TeamsHandler::handleGetTeam(
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

    int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid team ID");
        return;
    }

    try
    {
        auto team = m_teamService->getTeam(id, userId);
        if (!team)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Team not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(team->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении команды " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void TeamsHandler::handleCreateTeam(
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

    // Только супер-админ может создавать команды
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to create team"
        );
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Team team(nlohmannJson);

                    if (!team.caption.has_value() || team.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Caption is required");
                        return;
                    }

                    auto created = m_teamService->createTeam(team, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Failed to create team"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал команду id=" << *created->id
                        << ", caption=" << *created->caption;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании команды: " << e.what();
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

void TeamsHandler::handleUpdateTeam(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid team ID");
        return;
    }

    // Только супер-админ может обновлять команды
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to update team"
        );
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::Team team(nlohmannJson);

                    auto updated = m_teamService->updateTeam(team, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Team not found"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил команду id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении команды " << id << ": " << e.what();
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

void TeamsHandler::handleDeleteTeam(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid team ID");
        return;
    }

    // Только супер-админ может удалять команды
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to delete team"
        );
        return;
    }

    LOG_DEBUG << "DELETE /teams/" << id << " from user " << userId;

    try
    {
        if (m_teamService->deleteTeam(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил команду id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "Team not found"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении команды " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
