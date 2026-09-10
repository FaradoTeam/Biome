#include "common/log/log.h"

#include "api_client.h"

namespace terminal::api
{

namespace
{
/**
 * @brief Извлекает сообщение об ошибке из JSON-ответа.
 */
std::string extractErrorMessage(
    const web::json::value& json,
    const std::string& fallback
)
{
    try
    {
        if (json.has_field(U("message")))
        {
            return utility::conversions::to_utf8string(
                json.at(U("message")).as_string()
            );
        }
    }
    catch (...)
    {
        // Игнорируем — вернём fallback.
    }
    return fallback;
}
} // namespace

ApiClient::ApiClient(const std::string& baseUrl)
    : m_client(utility::conversions::to_string_t(baseUrl))
{
    LOG_DEBUG << "ApiClient создан для " << baseUrl;
}

std::future<LoginResult> ApiClient::login(
    const std::string& login,
    const std::string& password
)
{
    return std::async(
        std::launch::async,
        [this, login, password]() -> LoginResult
        {
            LoginResult result;

            web::json::value body;
            body[U("login")] = web::json::value::string(
                utility::conversions::to_string_t(login)
            );
            body[U("password")] = web::json::value::string(
                utility::conversions::to_string_t(password)
            );

            web::http::http_request request(web::http::methods::POST);
            request.set_request_uri(U("/api/v1/auth/login"));
            request.set_body(body);

            try
            {
                auto response = m_client.request(request).get();
                const auto status = response.status_code();

                web::json::value json;
                try
                {
                    json = response.extract_json().get();
                }
                catch (...)
                {
                    // Если тело не JSON — оставим пустой объект.
                }

                if (status == web::http::status_codes::OK)
                {
                    result.success = true;
                    result.accessToken = utility::conversions::to_utf8string(
                        json.at(U("access_token")).as_string()
                    );
                    if (json.has_field(U("token_type")))
                    {
                        result.tokenType = utility::conversions::to_utf8string(
                            json.at(U("token_type")).as_string()
                        );
                    }
                    if (json.has_field(U("expires_in")))
                    {
                        result.expiresIn = json.at(U("expires_in")).as_integer();
                    }
                }
                else
                {
                    result.error.code = static_cast<int>(status);
                    result.error.message = extractErrorMessage(
                        json, "Ошибка аутентификации"
                    );
                }
            }
            catch (const std::exception& e)
            {
                result.error.code = 0;
                result.error.message = std::string("Сетевая ошибка: ") + e.what();
                LOG_ERROR << "ApiClient::login: " << e.what();
            }

            return result;
        }
    );
}

std::future<LogoutResult> ApiClient::logout()
{
    return std::async(
        std::launch::async,
        [this]() -> LogoutResult
        {
            LogoutResult result;

            web::http::http_request request(web::http::methods::POST);
            request.set_request_uri(U("/api/v1/auth/logout"));

            if (!m_token.empty())
            {
                request.headers().add(
                    U("Authorization"),
                    utility::conversions::to_string_t("Bearer " + m_token)
                );
            }

            try
            {
                auto response = m_client.request(request).get();
                const auto status = response.status_code();

                if (status == web::http::status_codes::NoContent)
                {
                    result.success = true;
                }
                else
                {
                    result.error.code = static_cast<int>(status);

                    web::json::value json;
                    try
                    {
                        json = response.extract_json().get();
                    }
                    catch (...)
                    {
                    }

                    result.error.message = extractErrorMessage(
                        json, "Не удалось выйти из системы"
                    );
                }
            }
            catch (const std::exception& e)
            {
                result.error.code = 0;
                result.error.message = std::string("Сетевая ошибка: ") + e.what();
                LOG_ERROR << "ApiClient::logout: " << e.what();
            }

            return result;
        }
    );
}

} // namespace terminal::api
