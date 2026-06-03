#include <cpprest/uri.h>

#include "common/dto/user_team_role.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_team_roles_handler.h"

namespace server
{
namespace handlers
{

UserTeamRolesHandler::UserTeamRolesHandler(
    std::shared_ptr<services::IUserTeamRoleService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "UserTeamRolesHandler инициализирован без сервиса";
    }
}

void UserTeamRolesHandler::handleGetItems(
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

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::optional<int64_t> filterUserId;
    if (params.count("userId"))
        filterUserId = std::stoll(params["userId"]);

    std::optional<int64_t> teamId;
    if (params.count("teamId"))
        teamId = std::stoll(params["teamId"]);

    std::optional<int64_t> roleId;
    if (params.count("roleId"))
        roleId = std::stoll(params["roleId"]);

    auto pageData = m_service->getUserTeamRoles(
        page, pageSize, userId, filterUserId, teamId, roleId
    );

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < pageData.items.size(); ++i)
    {
        items[i] = dto::toWebJson(pageData.items[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = pageData.totalCount;
    response["page"] = page;
    response["pageSize"] = pageSize;

    request.reply(web::http::status_codes::OK, response);
}

void UserTeamRolesHandler::handleGetItem(
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

    auto item = m_service->getUserTeamRole(id, userId);
    if (!item)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "UserTeamRole not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(item->toJson())
    );
}

void UserTeamRolesHandler::handleCreateItem(
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

    // Только супер-админ может создавать назначения
    if (userId != 1)
    {
        web::http::http_response resp(web::http::status_codes::Forbidden);
        sendErrorResponse(
            resp,
            403,
            "Insufficient permissions to create UserTeamRole"
        );
        request.reply(resp);
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    dto::UserTeamRole utr(dto::toNlohmannJson(json));

                    if (!utr.userId || !utr.teamId || !utr.roleId)
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "userId, teamId and roleId are required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createUserTeamRole(utr, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Conflict);
                        sendErrorResponse(
                            resp,
                            409,
                            "User already has role in this team"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
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

void UserTeamRolesHandler::handleUpdateItem(
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

    // Только супер-админ может обновлять назначения
    if (userId != 1)
    {
        web::http::http_response resp(web::http::status_codes::Forbidden);
        sendErrorResponse(
            resp,
            403,
            "Insufficient permissions to update UserTeamRole"
        );
        request.reply(resp);
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::UserTeamRole utr(nl);

                    auto updated = m_service->updateUserTeamRole(utr, userId);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "UserTeamRole not found");
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
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

void UserTeamRolesHandler::handleDeleteItem(
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

    // Только супер-админ может удалять назначения
    if (userId != 1)
    {
        web::http::http_response resp(web::http::status_codes::Forbidden);
        sendErrorResponse(
            resp,
            403,
            "Insufficient permissions to delete UserTeamRole"
        );
        request.reply(resp);
        return;
    }

    if (m_service->deleteUserTeamRole(id, userId))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "UserTeamRole not found");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
