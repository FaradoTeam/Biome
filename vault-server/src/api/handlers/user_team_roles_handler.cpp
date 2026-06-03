#include <regex>

#include <cpprest/uri.h>

#include "common/dto/user_team_role.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_team_roles_handler.h"

namespace server::handlers
{

UserTeamRolesHandler::UserTeamRolesHandler(std::shared_ptr<services::IUserTeamRoleService> service)
    : m_service(std::move(service))
{
}

void UserTeamRolesHandler::handleGetItems(const web::http::http_request& request, const std::string& /*userId*/)
{
    auto params = extractQueryParams(request);
    int page = params.count("page") ? std::stoi(params["page"]) : 1;
    int pageSize = params.count("pageSize") ? std::stoi(params["pageSize"]) : 20;
    std::optional<int64_t> userId, teamId, roleId;
    if (params.count("userId"))
        userId = std::stoll(params["userId"]);
    if (params.count("teamId"))
        teamId = std::stoll(params["teamId"]);
    if (params.count("roleId"))
        roleId = std::stoll(params["roleId"]);

    auto pageData = m_service->getUserTeamRoles(page, pageSize, userId, teamId, roleId);
    web::json::value response;
    web::json::value items = web::json::value::array();
    for (size_t i = 0; i < pageData.items.size(); ++i)
        items[i] = dto::toWebJson(pageData.items[i].toJson());
    response["items"] = items;
    response["totalCount"] = pageData.totalCount;
    response["page"] = page;
    response["pageSize"] = pageSize;
    request.reply(web::http::status_codes::OK, response);
}

void UserTeamRolesHandler::handleGetItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    auto item = m_service->getUserTeamRole(id);
    if (!item)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "UserTeamRole not found");
        request.reply(resp);
        return;
    }
    request.reply(web::http::status_codes::OK, dto::toWebJson(item->toJson()));
}

void UserTeamRolesHandler::handleCreateItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    request
        .extract_json()
        .then(
            [this, request](pplx::task<web::json::value> task)
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
                    auto created = m_service->createUserTeamRole(utr);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Conflict);
                        sendErrorResponse(resp, 409, "User already has role in this team or invalid references");
                        request.reply(resp);
                        return;
                    }
                    request.reply(web::http::status_codes::Created, dto::toWebJson(created->toJson()));
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void UserTeamRolesHandler::handleUpdateItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    request
        .extract_json()
        .then(
            [this, request, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::UserTeamRole utr(nl);
                    auto updated = m_service->updateUserTeamRole(utr);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "UserTeamRole not found or update failed");
                        request.reply(resp);
                        return;
                    }
                    request.reply(web::http::status_codes::OK, dto::toWebJson(updated->toJson()));
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void UserTeamRolesHandler::handleDeleteItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    if (m_service->deleteUserTeamRole(id))
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

} // namespace server::handlers
