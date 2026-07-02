#include <cpprest/uri.h>

#include "common/dto/project_team.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "project_teams_handler.h"

namespace server::handlers
{

ProjectTeamsHandler::ProjectTeamsHandler(
    std::shared_ptr<services::IProjectTeamService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "ProjectTeamsHandler инициализирован без сервиса";
    }
}

void ProjectTeamsHandler::handleGetItems(
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

    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::optional<int64_t> projectId;
    if (params.count("projectId"))
        projectId = std::stoll(params["projectId"]);

    std::optional<int64_t> teamId;
    if (params.count("teamId"))
        teamId = std::stoll(params["teamId"]);

    LOG_DEBUG
        << "GET /project-teams: user=" << userIdOpt.value()
        << ", page=" << page << ", pageSize=" << pageSize
        << ", projectId=" << (projectId.has_value() ? std::to_string(*projectId) : "none")
        << ", teamId=" << (teamId.has_value() ? std::to_string(*teamId) : "none");

    try
    {
        auto pageData = m_service->getProjectTeams(page, pageSize, projectId, teamId);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.items.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.items[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка ProjectTeam: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ProjectTeamsHandler::handleGetItem(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid ID");
        return;
    }

    LOG_DEBUG
        << "GET /project-teams/" << id
        << " from user " << userIdOpt.value();

    try
    {
        auto item = m_service->getProjectTeam(id);
        if (!item)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "ProjectTeam not found");
            return;
        }

        sendJsonResponse(
            request,
            web::http::status_codes::OK,
            dto::toWebJson(item->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ProjectTeam " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ProjectTeamsHandler::handleCreateItem(
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

    LOG_DEBUG << "POST /project-teams from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    dto::ProjectTeam item(dto::toNlohmannJson(json));

                    if (!item.projectId.has_value() || !item.teamId.has_value())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "projectId and teamId are required"
                        );
                        return;
                    }

                    auto created = m_service->createProjectTeam(item, userId);
                    if (!created)
                    {
                        // Конфликт или недостаточно прав
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "ProjectTeam already exists or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал ProjectTeam: projectId=" << *created->projectId
                        << ", teamId=" << *created->teamId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ProjectTeam: " << e.what();
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

void ProjectTeamsHandler::handleDeleteItem(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid ID");
        return;
    }

    LOG_DEBUG
        << "DELETE /project-teams/" << id
        << " from user " << userId;

    try
    {
        auto result = m_service->deleteProjectTeam(id, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил ProjectTeam id=" << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ProjectTeam " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
