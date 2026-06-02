#include <regex>

#include <cpprest/uri.h>

#include "common/dto/role_menu_item.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "role_menu_items_handler.h"

namespace server::handlers
{

RoleMenuItemsHandler::RoleMenuItemsHandler(
    std::shared_ptr<services::IRoleMenuItemService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "RoleMenuItemsHandler инициализирован без сервиса";
    }
}

void RoleMenuItemsHandler::handleGetItems(
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

    std::optional<int64_t> roleId;
    if (params.count("roleId"))
        roleId = std::stoll(params["roleId"]);

    auto pageData = m_service->getRoleMenuItems(page, pageSize, roleId);

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

void RoleMenuItemsHandler::handleGetItem(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    int64_t id = extractId(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }

    auto item = m_service->getRoleMenuItem(id);
    if (!item)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RoleMenuItem not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(item->toJson())
    );
}

void RoleMenuItemsHandler::handleCreateItem(
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
    int64_t userId = *userIdOpt;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    dto::RoleMenuItem item(dto::toNlohmannJson(json));

                    if (!item.roleId.has_value()
                        || !item.caption.has_value()
                        || item.caption->empty()
                        || !item.link.has_value()
                        || item.link->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "roleId, caption and link are required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createRoleMenuItem(item, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to create RoleMenuItem");
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

void RoleMenuItemsHandler::handleUpdateItem(
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
    int64_t userId = *userIdOpt;

    int64_t id = extractId(request);
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
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::RoleMenuItem item(nl);

                    auto updated = m_service->updateRoleMenuItem(item, userId);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "RoleMenuItem not found or insufficient permissions");
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

void RoleMenuItemsHandler::handleDeleteItem(
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
    int64_t userId = *userIdOpt;

    int64_t id = extractId(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }

    if (m_service->deleteRoleMenuItem(id, userId))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RoleMenuItem not found or insufficient permissions");
        request.reply(resp);
    }
}

int64_t RoleMenuItemsHandler::extractId(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/role-menu-items/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
        return std::stoll(matches[1].str());
    return -1;
}

std::map<std::string, std::string> RoleMenuItemsHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
        params[p.first] = p.second;
    return params;
}

void RoleMenuItemsHandler::sendErrorResponse(
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

std::optional<int64_t> RoleMenuItemsHandler::parseUserId(
    const std::string& userIdStr,
    web::http::http_response& response
)
{
    if (userIdStr.empty())
    {
        sendErrorResponse(response, 401, "User not authenticated");
        return std::nullopt;
    }

    try
    {
        int64_t userId = std::stoll(userIdStr);
        if (userId <= 0)
        {
            sendErrorResponse(response, 400, "Invalid user ID");
            return std::nullopt;
        }
        return userId;
    }
    catch (const std::exception&)
    {
        sendErrorResponse(response, 400, "Invalid user ID format");
        return std::nullopt;
    }
}

} // namespace server::handlers
