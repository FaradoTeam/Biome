#include <regex>

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
    const std::string& /*userId*/
)
{
    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    auto [users, total] = m_userService->users(page, pageSize);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < users.size(); ++i)
    {
        items[i] = dto::toWebJson(users[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(total);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void UsersHandler::handleGetUser(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    int64_t id = extractUserIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
        return;
    }

    auto user = m_userService->user(id);
    if (!user)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "User not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(
            user->toJson()
        )
    );
}

void UsersHandler::handleCreateUser(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    request
        .extract_json()
        .then(
            [this, request](pplx::task<web::json::value> task)
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
                    if (!user.login || !user.email || password.empty())
                    {
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            resp,
                            400,
                            "Login, email and password are required"
                        );
                        request.reply(resp);
                        return;
                    }

                    auto created = m_userService->createUser(user, password);
                    if (!created)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Conflict
                        );
                        sendErrorResponse(
                            resp,
                            409,
                            "User with this login or email already exists"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(
                            created->toJson()
                        )
                    );
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(
                        web::http::status_codes::BadRequest
                    );
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void UsersHandler::handleUpdateUser(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractUserIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    nlohmann::json nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::User user(nlohmannJson);

                    auto updated = m_userService->updateUser(user);
                    if (!updated)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::NotFound
                        );
                        sendErrorResponse(
                            resp,
                            404,
                            "User not found or update failed"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(web::http::status_codes::NoContent);
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(
                        web::http::status_codes::BadRequest
                    );
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void UsersHandler::handleDeleteUser(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractUserIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
        return;
    }

    if (m_userService->deleteUser(id))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "User not found");
        request.reply(resp);
    }
}

int64_t UsersHandler::extractUserIdFromPath(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/users/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
    {
        return std::stoll(matches[1].str());
    }
    return -1;
}

std::map<std::string, std::string> UsersHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
    {
        params[p.first] = p.second;
    }
    return params;
}

void UsersHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace handlers
} // namespace server
