#include <cpprest/uri.h>

#include "common/dto/role.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "roles_handler.h"

namespace server::handlers
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

    std::string searchCaption;
    if (params.count("searchCaption"))
        searchCaption = params["searchCaption"];

    try
    {
        auto rolesPage = m_roleService->getRoles(page, pageSize, searchCaption);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < rolesPage.roles.size(); ++i)
        {
            items[i] = dto::toWebJson(rolesPage.roles[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(rolesPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка ролей: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RolesHandler::handleGetRole(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid role ID");
        return;
    }

    try
    {
        auto role = m_roleService->getRole(id);
        if (!role)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Role not found");
            return;
        }

        sendJsonResponse(
            request,
            web::http::status_codes::OK,
            dto::toWebJson(role->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении роли " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RolesHandler::handleCreateRole(
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

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Role role(nlohmannJson);

                    if (!role.caption.has_value() || role.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Caption is required");
                        return;
                    }

                    auto created = m_roleService->createRole(role, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create role"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал роль id=" << *created->id
                        << ", caption=" << *created->caption;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании роли: " << e.what();
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

void RolesHandler::handleUpdateRole(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid role ID");
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
                    dto::Role role(nlohmannJson);

                    auto updated = m_roleService->updateRole(role, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Role not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил роль id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении роли " << id << ": " << e.what();
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

void RolesHandler::handleDeleteRole(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid role ID");
        return;
    }

    LOG_DEBUG << "DELETE /roles/" << id << " from user " << userId;

    try
    {
        if (m_roleService->deleteRole(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил роль id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "Role not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении роли " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
