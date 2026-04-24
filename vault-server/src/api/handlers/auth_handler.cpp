#include <regex>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "common/log/log.h"

#include "common/dto/auth_request.h"
#include "common/dto/auth_response.h"
#include "common/dto/change_password_request.h"


#include "auth_handler.h"

namespace server
{
namespace handlers
{

AuthHandler::AuthHandler(
    std::shared_ptr<services::IAuthService> authService,
    std::shared_ptr<IAuthMiddleware> authMiddleware
)
    : m_authService(std::move(authService))
    , m_authMiddleware(std::move(authMiddleware))
{
    if (!m_authService)
    {
        LOG_WARN << "AuthHandler инициализирован без AuthService";
    }
    if (!m_authMiddleware)
    {
        LOG_WARN << "AuthHandler инициализирован без AuthMiddleware";
    }
}

void AuthHandler::handleLogin(const web::http::http_request& request)
{
    try
    {
        request
            .extract_json()
            .then(
                [this, request](pplx::task<web::json::value> task)
                {
                    web::json::value jsonBody;
                    try
                    {
                        jsonBody = task.get();
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        request.reply(response);
                        return;
                    }

                    // Используем DTO для парсинга запроса
                    dto::AuthRequest authRequest;
                    try
                    {
                        // Преобразуем web::json::value в nlohmann::json
                        auto jsonStr = jsonBody.serialize();
                        auto nlohmannJson = nlohmann::json::parse(jsonStr);
                        authRequest.fromJson(nlohmannJson);
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(response, 400, "Invalid request format");
                        request.reply(response);
                        return;
                    }

                    // Валидация запроса
                    if (!authRequest.isValid())
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            authRequest.validationError()
                        );
                        request.reply(response);
                        return;
                    }

                    // Вызываем сервис аутентификации
                    auto authResult = m_authService->login(
                        authRequest.login.value(),
                        authRequest.password.value()
                    );

                    if (!authResult.success)
                    {
                        web::http::http_response response(
                            static_cast<web::http::status_code>(authResult.errorCode)
                        );
                        sendErrorResponse(
                            response,
                            authResult.errorCode,
                            authResult.errorMessage
                        );
                        request.reply(response);
                        return;
                    }

                    // Формируем успешный ответ через DTO
                    dto::AuthResponse authResponse;
                    authResponse.accessToken = authResult.accessToken;

                    web::json::value responseJson;
                    responseJson["access_token"] = web::json::value::string(authResult.accessToken);
                    responseJson["token_type"] = web::json::value::string(authResult.tokenType);
                    responseJson["expires_in"] = web::json::value::number(authResult.expiresIn);

                    request.reply(web::http::status_codes::OK, responseJson);
                    LOG_INFO << "Пользователь успешно вошел в систему";
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(
            response,
            400,
            "Invalid request body: " + std::string(e.what())
        );
        request.reply(response);
    }
}

void AuthHandler::handleLogout(const web::http::http_request& request)
{
    auto authHeader = request.headers().find("Authorization");
    if (authHeader == request.headers().end())
    {
        LOG_ERROR << "Нет заголовка авторизации в запросе на выход из системы";
        web::json::value error;
        error["code"] = web::json::value::number(401);
        error["message"] = web::json::value::string("Missing Authorization header");
        request.reply(web::http::status_codes::Unauthorized, error);
        return;
    }

    std::regex bearerRegex(R"(^Bearer\s+([a-zA-Z0-9\-_\.]+)$)");
    std::smatch matches;

    if (std::regex_match(authHeader->second, matches, bearerRegex) && matches.size() > 1)
    {
        const std::string token = matches[1].str();

        // Вызываем сервис для выхода
        if (m_authService->logout(token))
        {
            request.reply(web::http::status_codes::NoContent);
            LOG_INFO << "Пользователь вышел из системы";
        }
        else
        {
            web::json::value error;
            error["code"] = web::json::value::number(500);
            error["message"] = web::json::value::string("Failed to logout");
            request.reply(web::http::status_codes::InternalError, error);
        }
    }
    else
    {
        LOG_ERROR << "Недопустимый формат заголовка авторизации при выходе из системы";
        web::json::value error;
        error["code"] = web::json::value::number(400);
        error["message"] = web::json::value::string("Invalid Authorization header format");
        request.reply(web::http::status_codes::BadRequest, error);
    }
}

void AuthHandler::handleChangePassword(
    const web::http::http_request& request,
    const std::string& userId
)
{
    if (userId.empty())
    {
        web::http::http_response response(web::http::status_codes::Unauthorized);
        sendErrorResponse(response, 401, "User not authenticated");
        request.reply(response);
        return;
    }

    try
    {
        request
            .extract_json()
            .then(
                [this, request, userId](pplx::task<web::json::value> task)
                {
                    web::json::value jsonBody;
                    try
                    {
                        jsonBody = task.get();
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        request.reply(response);
                        return;
                    }

                    // Парсим запрос через DTO
                    dto::ChangePasswordRequest changeRequest;
                    try
                    {
                        auto jsonStr = jsonBody.serialize();
                        auto nlohmannJson = nlohmann::json::parse(jsonStr);
                        changeRequest.fromJson(nlohmannJson);
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(response, 400, "Invalid request format");
                        request.reply(response);
                        return;
                    }

                    // Валидация запроса
                    if (!changeRequest.isValid())
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            response,
                            400,
                            changeRequest.validationError()
                        );
                        request.reply(response);
                        return;
                    }

                    // Конвертируем userId из строки в число
                    int64_t userIdInt;
                    try
                    {
                        userIdInt = std::stoll(userId);
                    }
                    catch (const std::exception& e)
                    {
                        web::http::http_response response(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(response, 400, "Invalid user ID");
                        request.reply(response);
                        return;
                    }

                    // Вызываем сервис смены пароля
                    auto result = m_authService->changePassword(
                        userIdInt,
                        changeRequest.oldPassword.value(),
                        changeRequest.newPassword.value()
                    );

                    if (!result.success)
                    {
                        web::http::http_response response(
                            static_cast<web::http::status_code>(result.errorCode)
                        );
                        sendErrorResponse(
                            response,
                            result.errorCode,
                            result.errorMessage
                        );
                        request.reply(response);
                        return;
                    }

                    request.reply(web::http::status_codes::NoContent);
                    LOG_INFO << "Пароль изменен для пользователя " << userId;
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        web::http::http_response response(web::http::status_codes::BadRequest);
        sendErrorResponse(
            response,
            400,
            "Invalid request body: " + std::string(e.what())
        );
        request.reply(response);
    }
}

void AuthHandler::sendErrorResponse(
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
