#include <regex>

#include <cpprest/uri.h>

#include "common/dto/role.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "roles_handler.h"

namespace server
{
namespace handlers
{

RolesHandler::RolesHandler(std::shared_ptr<services::IRoleService> roleService)
    : m_roleService(std::move(roleService))
{
    if (!m_roleService)
    {
        LOG_WARN << "RolesHandler инициализирован без RoleService";
    }
}

void RolesHandler::handleGetRoles(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::string searchCaption;
    if (params.count("searchCaption"))
        searchCaption = params["searchCaption"];

    auto rolesPage = m_roleService->getRoles(page, pageSize, searchCaption);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < rolesPage.roles.size(); ++i)
    {
        items[i] = dto::toWebJson(rolesPage.roles[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(rolesPage.totalCount);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void RolesHandler::handleGetRole(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    int64_t id = extractRoleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid role ID");
        request.reply(resp);
        return;
    }

    auto role = m_roleService->getRole(id);
    if (!role)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Role not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(role->toJson())
    );
}

void RolesHandler::handleCreateRole(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    request
        .extract_json()
        .then(
            [this, request](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Role role(nlohmannJson);

                    if (!role.caption || role.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_roleService->createRole(role);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Could not create role");
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

void RolesHandler::handleUpdateRole(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractRoleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid role ID");
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
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::Role role(nlohmannJson);

                    auto updated = m_roleService->updateRole(role);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "Role not found or update failed");
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

void RolesHandler::handleDeleteRole(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractRoleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid role ID");
        request.reply(resp);
        return;
    }

    if (m_roleService->deleteRole(id))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Role not found");
        request.reply(resp);
    }
}

int64_t RolesHandler::extractRoleIdFromPath(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/roles/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
    {
        return std::stoll(matches[1].str());
    }
    return -1;
}

std::map<std::string, std::string> RolesHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
    {
        params[p.first] = p.second;
    }
    return params;
}

void RolesHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace handlers
} // namespace server
