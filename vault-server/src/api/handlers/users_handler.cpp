#include <cpprest/uri.h>

#include "common/dto/user.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "users_handler.h"

namespace server
{
namespace handlers
{

UsersHandler::UsersHandler(std::shared_ptr<services::IUserService> userService)
    : m_userService(std::move(userService))
{
    if (!m_userService)
    {
        LOG_WARN << "UsersHandler инициализирован без UserService";
    }
}

void UsersHandler::handleGetUsers(
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

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetUsers: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetUsers: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Параметры фильтрации
    std::string login;
    if (params.count("login"))
    {
        login = params["login"];
    }

    std::string name;
    if (params.count("name"))
    {
        name = params["name"];
    }

    std::string email;
    if (params.count("email"))
    {
        email = params["email"];
    }

    std::optional<bool> isBlocked;
    if (params.count("isBlocked"))
    {
        isBlocked = parseBool(params["isBlocked"]);
    }

    LOG_DEBUG
        << "GET /users: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", login=" << login
        << ", name=" << name
        << ", email=" << email
        << ", isBlocked=" << (isBlocked.has_value() ? (*isBlocked ? "true" : "false") : "none");

    try
    {
        auto usersPage = m_userService->users(page, pageSize, userId, login, name, email, isBlocked);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < usersPage.users.size(); ++i)
        {
            items[i] = dto::toWebJson(usersPage.users[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(usersPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка пользователей: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void UsersHandler::handleGetUser(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    try
    {
        auto user = m_userService->user(id, userId);
        if (!user)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "User not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(user->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении пользователя " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void UsersHandler::handleCreateUser(
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

    // Только супер-админ может создавать пользователей
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to create user"
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
                    auto jsonBody = task.get();

                    dto::User user(dto::toNlohmannJson(jsonBody));

                    // Извлекаем пароль отдельно (он не является частью DTO User)
                    std::string password;
                    if (jsonBody.has_field("password"))
                    {
                        password = utility::conversions::to_utf8string(
                            jsonBody.at("password").as_string()
                        );
                    }

                    // Валидация обязательных полей
                    if (!user.login.has_value()
                        || !user.email.has_value()
                        || user.login->empty()
                        || user.email->empty()
                        || password.empty())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Login, email and password are required"
                        );
                        return;
                    }

                    auto created = m_userService->createUser(user, password, userId);
                    if (!created)
                    {
                        // Конфликт (дубликат)
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Conflict,
                            "User with this login or email already exists"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал пользователя id=" << *created->id
                        << ", login=" << *created->login;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании пользователя: " << e.what();
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

void UsersHandler::handleUpdateUser(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    // Только супер-админ может обновлять пользователей
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to update user"
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
                    auto jsonBody = task.get();
                    nlohmann::json nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::User user(nlohmannJson);

                    auto updated = m_userService->updateUser(user, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "User not found"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил пользователя id=" << id;

                    // Возвращаем NoContent вместо JSON
                    web::http::http_response response(web::http::status_codes::NoContent);
                    sendResponse(request, response);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении пользователя " << id << ": " << e.what();
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

void UsersHandler::handleDeleteUser(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    // Только супер-админ может удалять пользователей
    if (userId != 1)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Forbidden,
            "Insufficient permissions to delete user"
        );
        return;
    }

    LOG_DEBUG << "DELETE /users/" << id << " from user " << userId;

    try
    {
        if (m_userService->deleteUser(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил пользователя id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "User not found"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении пользователя " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
