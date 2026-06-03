#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irole_service.h"

#include "base_handler.h"

namespace server::handlers
{

class RolesHandler final : public BaseHandler
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
    std::shared_ptr<services::IRoleService> m_roleService;
};

} // namespace server::handlers
