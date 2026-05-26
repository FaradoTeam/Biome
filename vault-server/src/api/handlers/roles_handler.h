#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irole_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с ролями.
 */
class RolesHandler final
{
public:
    explicit RolesHandler(std::shared_ptr<services::IRoleService> roleService);

    void handleGetRoles(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetRole(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateRole(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateRole(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteRole(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractRoleIdFromPath(const web::http::http_request& request);
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

private:
    std::shared_ptr<services::IRoleService> m_roleService;
};

} // namespace handlers
} // namespace server
