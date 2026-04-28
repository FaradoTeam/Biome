#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_service.h"

namespace server
{
namespace handlers
{

class UsersHandler final
{
public:
    explicit UsersHandler(std::shared_ptr<services::IUserService> userService);

    void handleGetUsers(const web::http::http_request& request, const std::string& userId);
    void handleGetUser(const web::http::http_request& request, const std::string& userId);
    void handleCreateUser(const web::http::http_request& request, const std::string& userId);
    void handleUpdateUser(const web::http::http_request& request, const std::string& userId);
    void handleDeleteUser(const web::http::http_request& request, const std::string& userId);

private:
    int64_t extractUserIdFromPath(const web::http::http_request& request);
    std::map<std::string, std::string> extractQueryParams(const web::http::http_request& request);
    void sendErrorResponse(web::http::http_response& response, int code, const std::string& message);

private:
    std::shared_ptr<services::IUserService> m_userService;
};

} // namespace handlers
} // namespace server
