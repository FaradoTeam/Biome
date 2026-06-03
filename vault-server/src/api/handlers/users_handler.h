#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

class UsersHandler final : public BaseHandler
{
public:
    explicit UsersHandler(std::shared_ptr<services::IUserService> userService);

    void handleGetUsers(const web::http::http_request& request, const std::string& userId);
    void handleGetUser(const web::http::http_request& request, const std::string& userId);
    void handleCreateUser(const web::http::http_request& request, const std::string& userId);
    void handleUpdateUser(const web::http::http_request& request, const std::string& userId);
    void handleDeleteUser(const web::http::http_request& request, const std::string& userId);

private:
    std::shared_ptr<services::IUserService> m_userService;
};

} // namespace handlers
} // namespace server
