#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_team_role_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

class UserTeamRolesHandler final : public BaseHandler
{
public:
    explicit UserTeamRolesHandler(std::shared_ptr<services::IUserTeamRoleService> service);
    void handleGetItems(const web::http::http_request& request, const std::string& userId);
    void handleGetItem(const web::http::http_request& request, const std::string& userId);
    void handleCreateItem(const web::http::http_request& request, const std::string& userId);
    void handleUpdateItem(const web::http::http_request& request, const std::string& userId);
    void handleDeleteItem(const web::http::http_request& request, const std::string& userId);

private:
    std::shared_ptr<services::IUserTeamRoleService> m_service;
};

} // namespace handlers
} // namespace server
