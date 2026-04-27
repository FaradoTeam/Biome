#include <regex>

#include <cpprest/uri.h>

#include "common/dto/user.h"
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

std::map<std::string, std::string> UsersHandler::extractQueryParams(const web::http::http_request& request)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
        params[p.first] = p.second;
    return params;
}

void UsersHandler::sendErrorResponse(web::http::http_response& response, int code, const std::string& message)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
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

void UsersHandler::handleGetUsers(const web::http::http_request& request, const std::string& /*userId*/)
{
    auto params = extractQueryParams(request);
    int page = 1, pageSize = 20;
    if (params.count("page"))
        page = std::stoi(params["page"]);
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    auto [users, total] = m_userService->getUsers(page, pageSize);

    web::json::value response;
    web::json::value items = web::json::value::array();
    for (size_t i = 0; i < users.size(); ++i)
    {
        // Преобразуем наш DTO в JSON. Так как у нас есть метод toJson() для User,
        // можно воспользоваться им, но проще сразу в json::value.
        web::json::value item;
        const auto& u = users[i];
        item["id"] = web::json::value::number(u.id.value_or(0));
        if (u.login)
            item["login"] = web::json::value::string(*u.login);
        if (u.firstName)
            item["firstName"] = web::json::value::string(*u.firstName);
        if (u.middleName)
            item["middleName"] = web::json::value::string(*u.middleName);
        if (u.lastName)
            item["lastName"] = web::json::value::string(*u.lastName);
        if (u.email)
            item["email"] = web::json::value::string(*u.email);
        item["needChangePassword"] = web::json::value::boolean(u.needChangePassword.value_or(false));
        item["isBlocked"] = web::json::value::boolean(u.isBlocked.value_or(false));
        item["isSuperAdmin"] = web::json::value::boolean(u.isSuperAdmin.value_or(false));
        item["isHidden"] = web::json::value::boolean(u.isHidden.value_or(false));
        items[i] = item;
    }
    response["items"] = items;
    response["totalCount"] = web::json::value::number(total);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);
    request.reply(web::http::status_codes::OK, response);
}

void UsersHandler::handleGetUser(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractUserIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
        return;
    }
    auto user = m_userService->getUser(id);
    if (!user)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "User not found");
        request.reply(resp);
        return;
    }

    web::json::value resp;
    const auto& u = *user;
    resp["id"] = web::json::value::number(u.id.value_or(0));
    if (u.login)
        resp["login"] = web::json::value::string(*u.login);
    if (u.firstName)
        resp["firstName"] = web::json::value::string(*u.firstName);
    if (u.middleName)
        resp["middleName"] = web::json::value::string(*u.middleName);
    if (u.lastName)
        resp["lastName"] = web::json::value::string(*u.lastName);
    if (u.email)
        resp["email"] = web::json::value::string(*u.email);
    resp["needChangePassword"] = web::json::value::boolean(u.needChangePassword.value_or(false));
    resp["isBlocked"] = web::json::value::boolean(u.isBlocked.value_or(false));
    resp["isSuperAdmin"] = web::json::value::boolean(u.isSuperAdmin.value_or(false));
    resp["isHidden"] = web::json::value::boolean(u.isHidden.value_or(false));

    request.reply(web::http::status_codes::OK, resp);
}

void UsersHandler::handleCreateUser(const web::http::http_request& request, const std::string& /*userId*/)
{
    request.extract_json().then([this, request](pplx::task<web::json::value> task)
                                {
        try {
            auto jsonBody = task.get();
            dto::User user;
            // Парсим JSON в DTO
            if (jsonBody.has_field("login")) user.login = jsonBody.at("login").as_string();
            if (jsonBody.has_field("firstName")) user.firstName = jsonBody.at("firstName").as_string();
            if (jsonBody.has_field("middleName")) user.middleName = jsonBody.at("middleName").as_string();
            if (jsonBody.has_field("lastName")) user.lastName = jsonBody.at("lastName").as_string();
            if (jsonBody.has_field("email")) user.email = jsonBody.at("email").as_string();
            // Пароль обязателен при создании
            std::string password;
            if (jsonBody.has_field("password")) password = jsonBody.at("password").as_string();

            if (!user.login || !user.email || password.empty()) {
                web::http::http_response resp(web::http::status_codes::BadRequest);
                sendErrorResponse(resp, 400, "Login, email and password are required");
                request.reply(resp);
                return;
            }

            auto created = m_userService->createUser(user, password);
            if (!created) {
                web::http::http_response resp(web::http::status_codes::Conflict);
                sendErrorResponse(resp, 409, "User with this login or email already exists");
                request.reply(resp);
                return;
            }

            // Возвращаем созданного пользователя (без пароля)
            web::json::value resp;
            const auto& u = *created;
            resp["id"] = web::json::value::number(u.id.value_or(0));
            if (u.login) resp["login"] = web::json::value::string(*u.login);
            if (u.firstName) resp["firstName"] = web::json::value::string(*u.firstName);
            if (u.middleName) resp["middleName"] = web::json::value::string(*u.middleName);
            if (u.lastName) resp["lastName"] = web::json::value::string(*u.lastName);
            if (u.email) resp["email"] = web::json::value::string(*u.email);
            resp["needChangePassword"] = web::json::value::boolean(u.needChangePassword.value_or(false));
            resp["isBlocked"] = web::json::value::boolean(u.isBlocked.value_or(false));
            resp["isSuperAdmin"] = web::json::value::boolean(u.isSuperAdmin.value_or(false));
            resp["isHidden"] = web::json::value::boolean(u.isHidden.value_or(false));
            request.reply(web::http::status_codes::Created, resp);
        } catch (const std::exception& e) {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
            request.reply(resp);
        } })
        .wait();
}

void UsersHandler::handleUpdateUser(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractUserIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
        return;
    }
    request.extract_json().then([this, request, id](pplx::task<web::json::value> task)
                                {
        try {
            auto jsonBody = task.get();
            dto::User user;
            user.id = id;
            // Обновляем только переданные поля
            if (jsonBody.has_field("firstName")) user.firstName = jsonBody.at("firstName").as_string();
            if (jsonBody.has_field("middleName")) user.middleName = jsonBody.at("middleName").as_string();
            if (jsonBody.has_field("lastName")) user.lastName = jsonBody.at("lastName").as_string();
            if (jsonBody.has_field("email")) user.email = jsonBody.at("email").as_string();
            if (jsonBody.has_field("needChangePassword")) user.needChangePassword = jsonBody.at("needChangePassword").as_bool();
            if (jsonBody.has_field("isBlocked")) user.isBlocked = jsonBody.at("isBlocked").as_bool();
            if (jsonBody.has_field("isSuperAdmin")) user.isSuperAdmin = jsonBody.at("isSuperAdmin").as_bool();
            if (jsonBody.has_field("isHidden")) user.isHidden = jsonBody.at("isHidden").as_bool();

            auto updated = m_userService->updateUser(user);
            if (!updated) {
                web::http::http_response resp(web::http::status_codes::NotFound);
                sendErrorResponse(resp, 404, "User not found or update failed");
                request.reply(resp);
                return;
            }
            request.reply(web::http::status_codes::NoContent);
        } catch (const std::exception& e) {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
            request.reply(resp);
        } })
        .wait();
}

void UsersHandler::handleDeleteUser(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractUserIdFromPath(request);
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

} // namespace handlers
} // namespace server
