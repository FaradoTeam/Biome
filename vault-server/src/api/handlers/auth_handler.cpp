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

AuthHandler::AuthHandler(std::shared_ptr<services::IAuthService> authService)
    : m_authService(std::move(authService))
{
    if (!m_authService)
    {
        LOG_WARN << "AuthHandler инициализирован без AuthService";
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
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        return;
                    }

                    dto::AuthRequest authRequest;
                    try
                    {
                        auto jsonStr = jsonBody.serialize();
                        auto nlohmannJson = nlohmann::json::parse(
                            utility::conversions::to_utf8string(jsonStr)
                        );
                        authRequest.fromJson(nlohmannJson);
                    }
                    catch (const std::exception& e)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Invalid request format"
                        );
                        return;
                    }

                    if (!authRequest.isValid())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            authRequest.validationError()
                        );
                        return;
                    }

                    auto authResult = m_authService->login(
                        authRequest.login.value(),
                        authRequest.password.value()
                    );

                    if (!authResult.success)
                    {
                        sendErrorResponse(
                            request,
                            static_cast<web::http::status_code>(authResult.errorCode),
                            authResult.errorMessage
                        );
                        return;
                    }

                    web::json::value responseJson;
                    responseJson[U("access_token")] = web::json::value::string(
                        utility::conversions::to_string_t(authResult.accessToken)
                    );
                    responseJson[U("token_type")] = web::json::value::string(
                        utility::conversions::to_string_t(authResult.tokenType)
                    );
                    responseJson[U("expires_in")] = web::json::value::number(
                        authResult.expiresIn
                    );

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        responseJson
                    );

                    LOG_INFO << "Пользователь успешно вошел в систему";
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::BadRequest,
            "Invalid request body: " + std::string(e.what())
        );
    }
}

void AuthHandler::handleLogout(const web::http::http_request& request)
{
    auto authHeader = request.headers().find("Authorization");
    if (authHeader == request.headers().end())
    {
        LOG_ERROR << "Нет заголовка авторизации в запросе на выход из системы";
        sendErrorResponse(
            request,
            web::http::status_codes::Unauthorized,
            "Missing Authorization header"
        );
        return;
    }

    std::regex bearerRegex(R"(^Bearer\s+([a-zA-Z0-9\-_\.]+)$)");
    std::smatch matches;

    if (std::regex_match(authHeader->second, matches, bearerRegex) && matches.size() > 1)
    {
        const std::string token = matches[1].str();

        if (m_authService->logout(token))
        {
            // NoContent
            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
            LOG_INFO << "Пользователь вышел из системы";
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::InternalError,
                "Failed to logout"
            );
        }
    }
    else
    {
        LOG_ERROR << "Недопустимый формат заголовка авторизации при выходе из системы";
        sendErrorResponse(
            request,
            web::http::status_codes::BadRequest,
            "Invalid Authorization header format"
        );
    }
}

void AuthHandler::handleChangePassword(
    const web::http::http_request& request,
    const std::string& userId
)
{
    if (userId.empty())
    {
        sendErrorResponse(
            request,
            web::http::status_codes::Unauthorized,
            "User not authenticated"
        );
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
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Invalid JSON: " + std::string(e.what())
                        );
                        return;
                    }

                    dto::ChangePasswordRequest changeRequest;
                    try
                    {
                        auto jsonStr = jsonBody.serialize();
                        auto nlohmannJson = nlohmann::json::parse(
                            utility::conversions::to_utf8string(jsonStr)
                        );
                        changeRequest.fromJson(nlohmannJson);
                    }
                    catch (const std::exception& e)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Invalid request format"
                        );
                        return;
                    }

                    if (!changeRequest.isValid())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            changeRequest.validationError()
                        );
                        return;
                    }

                    int64_t userIdInt;
                    try
                    {
                        userIdInt = std::stoll(userId);
                    }
                    catch (const std::exception& e)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Invalid user ID"
                        );
                        return;
                    }

                    auto result = m_authService->changePassword(
                        userIdInt,
                        changeRequest.oldPassword.value(),
                        changeRequest.newPassword.value()
                    );

                    if (!result.success)
                    {
                        sendErrorResponse(
                            request,
                            static_cast<web::http::status_code>(result.errorCode),
                            result.errorMessage
                        );
                        return;
                    }

                    // NoContent
                    web::http::http_response response(web::http::status_codes::NoContent);
                    sendResponse(request, response);

                    LOG_INFO << "Пароль изменен для пользователя " << userId;
                }
            )
            .wait();
    }
    catch (const std::exception& e)
    {
        sendErrorResponse(
            request,
            web::http::status_codes::BadRequest,
            "Invalid request body: " + std::string(e.what())
        );
    }
}

} // namespace handlers
} // namespace server
