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

    try
    {
        auto pageData = m_service->getUserTeamRoles(
            page, pageSize, userId, filterUserId, teamId, roleId
        );

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
        LOG_ERROR << "Ошибка при получении списка UserTeamRole: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void UserTeamRolesHandler::handleGetItem(
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

    try
    {
        auto item = m_service->getUserTeamRole(id, userId);
        if (!item)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "UserTeamRole not found");
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
        LOG_ERROR << "Ошибка при получении UserTeamRole " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void UserTeamRolesHandler::handleCreateItem(
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

    // Только супер-админ может создавать назначения
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to create UserTeamRole"
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
                    auto json = task.get();
                    dto::UserTeamRole utr(dto::toNlohmannJson(json));

                    if (!utr.userId.has_value() || !utr.teamId.has_value() || !utr.roleId.has_value())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "userId, teamId and roleId are required"
                        );
                        return;
                    }

                    auto created = m_service->createUserTeamRole(utr, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Conflict,
                            "User already has role in this team"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал UserTeamRole id=" << *created->id
                        << ", userId=" << *created->userId
                        << ", teamId=" << *created->teamId
                        << ", roleId=" << *created->roleId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании UserTeamRole: " << e.what();
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

void UserTeamRolesHandler::handleUpdateItem(
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

    // Только супер-админ может обновлять назначения
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to update UserTeamRole"
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
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::UserTeamRole utr(nl);

                    auto updated = m_service->updateUserTeamRole(utr, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "UserTeamRole not found"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил UserTeamRole id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении UserTeamRole: " << e.what();
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

void UserTeamRolesHandler::handleDeleteItem(
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

    // Только супер-админ может удалять назначения
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to delete UserTeamRole"
        );
        return;
    }

    LOG_DEBUG << "DELETE /user-team-roles/" << id << " from user " << userId;

    try
    {
        if (m_service->deleteUserTeamRole(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил UserTeamRole id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "UserTeamRole not found"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении UserTeamRole " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
