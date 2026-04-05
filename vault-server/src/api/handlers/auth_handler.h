#pragma once

#include <memory>
#include <string>

#include "../middleware/auth_middleware.h"

namespace httplib
{
class Request;
class Response;
}

namespace farado
{
namespace server
{

// class AuthMiddleware;

namespace handlers
{

class AuthHandler final
{
public:
    explicit AuthHandler(std::shared_ptr<AuthMiddleware> authMiddleware);
    ~AuthHandler() = default;

    /**
     * Обрабатывает запрос на вход
     */
    void handleLogin(const httplib::Request& req, httplib::Response& res);

    /**
     * Обрабатывает запрос на выход
     */
    void handleLogout(const httplib::Request& req, httplib::Response& res);

    /**
     * Обрабатывает запрос на смену пароля
     */
    void handleChangePassword(const httplib::Request& req, httplib::Response& res, const std::string& userId);

private:
    std::string validateCredentials(const std::string& login, const std::string& password);

private:
    std::shared_ptr<AuthMiddleware> m_authMiddleware;
};

} // namespace handlers
} // namespace server
} // namespace farado
