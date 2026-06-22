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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка ProjectTeam: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ProjectTeamsHandler::handleGetItem(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
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
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "ProjectTeam not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(item->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ProjectTeam " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ProjectTeamsHandler::handleCreateItem(
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
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(resp, 400, "projectId and teamId are required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createProjectTeam(item, userId);
                    if (!created)
                    {
                        // Конфликт или недостаточно прав
                        web::http::http_response resp(
                            web::http::status_codes::Forbidden
                        );
                        sendErrorResponse(
                            resp,
                            403,
                            "ProjectTeam already exists or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал ProjectTeam: projectId=" << *created->projectId
                        << ", teamId=" << *created->teamId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ProjectTeam: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
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
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил ProjectTeam id=" << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ProjectTeam " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace server::handlers
