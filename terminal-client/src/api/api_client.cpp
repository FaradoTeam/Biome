#include <cpprest/uri_builder.h>

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

/**
 * @brief Парсит JSON-объект в модели::User.
 */
terminal::models::User parseUser(const web::json::value& json)
{
    using namespace terminal::models;

    auto getStr = [](const web::json::value& j, const utility::char_t* key)
        -> std::optional<std::string>
    {
        if (j.has_field(key) && !j.at(key).is_null())
        {
            return utility::conversions::to_utf8string(j.at(key).as_string());
        }
        return std::nullopt;
    };

    auto getInt = [](const web::json::value& j, const utility::char_t* key)
        -> std::optional<int64_t>
    {
        if (j.has_field(key) && !j.at(key).is_null())
        {
            return j.at(key).as_number().to_int64();
        }
        return std::nullopt;
    };

    auto getBool = [](const web::json::value& j, const utility::char_t* key)
        -> std::optional<bool>
    {
        if (j.has_field(key) && !j.at(key).is_null())
        {
            return j.at(key).as_bool();
        }
        return std::nullopt;
    };

    User u;
    u.id = getInt(json, U("id"));
    u.login = getStr(json, U("login"));
    u.firstName = getStr(json, U("firstName"));
    u.middleName = getStr(json, U("middleName"));
    u.lastName = getStr(json, U("lastName"));
    u.email = getStr(json, U("email"));
    u.needChangePassword = getBool(json, U("needChangePassword"));
    u.isBlocked = getBool(json, U("isBlocked"));
    u.isSuperAdmin = getBool(json, U("isSuperAdmin"));
    u.isHidden = getBool(json, U("isHidden"));
    return u;
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

std::future<UsersResult> ApiClient::getUsers(const models::UserQuery& query)
{
    return std::async(
        std::launch::async,
        [this, query]() -> UsersResult
        {
            UsersResult result;

            web::uri_builder builder(U("/api/v1/users"));
            builder.append_query(U("page"), query.page);
            builder.append_query(U("pageSize"), query.pageSize);

            if (!query.login.empty())
                builder.append_query(U("login"), utility::conversions::to_string_t(query.login));

            if (!query.name.empty())
                builder.append_query(U("name"), utility::conversions::to_string_t(query.name));

            if (!query.email.empty())
                builder.append_query(U("email"), utility::conversions::to_string_t(query.email));

            if (query.isBlocked.has_value())
                builder.append_query(U("isBlocked"), *query.isBlocked ? U("true") : U("false"));

            web::http::http_request request(web::http::methods::GET);
            request.set_request_uri(builder.to_uri());

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

                web::json::value json;
                try
                {
                    json = response.extract_json().get();
                }
                catch (...)
                {
                }

                if (status == web::http::status_codes::OK)
                {
                    result.success = true;
                    if (json.has_field(U("totalCount")))
                        result.page.totalCount = json.at(U("totalCount")).as_number().to_int64();
                    if (json.has_field(U("page")))
                        result.page.page = json.at(U("page")).as_integer();
                    if (json.has_field(U("pageSize")))
                        result.page.pageSize = json.at(U("pageSize")).as_integer();

                    if (json.has_field(U("items")) && json.at(U("items")).is_array())
                    {
                        for (const auto& item : json.at(U("items")).as_array())
                        {
                            result.page.items.push_back(parseUser(item));
                        }
                    }
                }
                else
                {
                    result.error.code = static_cast<int>(status);
                    result.error.message = extractErrorMessage(
                        json, "Не удалось получить список пользователей"
                    );
                }
            }
            catch (const std::exception& e)
            {
                result.error.code = 0;
                result.error.message = std::string("Сетевая ошибка: ") + e.what();
                LOG_ERROR << "ApiClient::getUsers: " << e.what();
            }

            return result;
        }
    );
}

} // namespace terminal::api
