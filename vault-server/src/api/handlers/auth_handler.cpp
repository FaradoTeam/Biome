#include <iostream>
#include <regex>

#include <httplib.h>

#include <nlohmann/json.hpp>

#include "common/log/log.h"

#include "auth_handler.h"

#include "../middleware/auth_middleware.h"

namespace farado
{
namespace server
{
namespace handlers
{

AuthHandler::AuthHandler(std::shared_ptr<AuthMiddleware> authMiddleware)
    : m_authMiddleware(authMiddleware)
{
    if (!m_authMiddleware)
    {
        LOG_ERROR << "Warning: AuthHandler initialized without AuthMiddleware";
    }
}

void AuthHandler::handleLogin(const httplib::Request& req, httplib::Response& res)
{
    try
    {
        // Парсим тело запроса
        auto jsonBody = nlohmann::json::parse(req.body);

        std::string login = jsonBody.value("login", "");
        std::string password = jsonBody.value("password", "");

        // Валидация входных данных
        if (login.empty() || password.empty())
        {
            nlohmann::json error;
            error["code"] = 400;
            error["message"] = "Login and password are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        // Проверяем учетные данные
        std::string userId = validateCredentials(login, password);
        if (userId.empty())
        {
            nlohmann::json error;
            error["code"] = 401;
            error["message"] = "Invalid credentials";
            res.status = 401;
            res.set_content(error.dump(), "application/json");
            return;
        }

        // Генерируем JWT токен
        std::string token = m_authMiddleware->generateToken(userId);

        // Формируем ответ
        nlohmann::json response;
        response["access_token"] = token;
        response["token_type"] = "Bearer";
        response["expires_in"] = 3600;

        res.set_content(response.dump(), "application/json");
        LOG_INFO << "User " << userId << " logged in successfully";
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid request body: " + std::string(e.what());
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

void AuthHandler::handleLogout(const httplib::Request& req, httplib::Response& res)
{
    // Извлекаем токен из заголовка
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty())
    {
        res.status = 401;
        return;
    }

    // Извлекаем Bearer токен
    std::regex bearerRegex(R"(^Bearer\s+([a-zA-Z0-9\-_\.]+)$)");
    std::smatch matches;
    std::string token;

    if (std::regex_match(authHeader, matches, bearerRegex) && matches.size() > 1)
    {
        token = matches[1].str();
        m_authMiddleware->invalidateToken(token);
    }

    res.status = 204;
    LOG_INFO << "User logged out";
}

void AuthHandler::handleChangePassword(
    const httplib::Request& req,
    httplib::Response& res,
    const std::string& userId
)
{
    try
    {
        auto jsonBody = nlohmann::json::parse(req.body);

        std::string oldPassword = jsonBody.value("oldPassword", "");
        std::string newPassword = jsonBody.value("newPassword", "");

        if (oldPassword.empty() || newPassword.empty())
        {
            nlohmann::json error;
            error["code"] = 400;
            error["message"] = "Old password and new password are required";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        if (newPassword.length() < 8)
        {
            nlohmann::json error;
            error["code"] = 400;
            error["message"] = "New password must be at least 8 characters long";
            res.status = 400;
            res.set_content(error.dump(), "application/json");
            return;
        }

        // Здесь должна быть логика смены пароля в БД
        // validate old password for userId, then update with new password hash

        res.status = 204;
        LOG_INFO << "Password changed for user " << userId;
    }
    catch (const std::exception& e)
    {
        nlohmann::json error;
        error["code"] = 400;
        error["message"] = "Invalid request body: " + std::string(e.what());
        res.status = 400;
        res.set_content(error.dump(), "application/json");
    }
}

std::string AuthHandler::validateCredentials(const std::string& login, const std::string& password)
{
    // TODO: Реализовать проверку в БД
    // Временная заглушка для тестирования
    if (login == "admin" && password == "admin123")
    {
        return "1";
    }

    return "";
}

} // namespace handlers
} // namespace server
} // namespace farado
