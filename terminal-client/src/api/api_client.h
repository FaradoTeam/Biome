#pragma once

#include <future>
#include <string>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

namespace terminal::api
{

/**
 * @brief Структура ошибки от API.
 */
struct ApiError final
{
    int code { 0 }; ///< HTTP-код либо 0 при сетевой ошибке
    std::string message; ///< Текст ошибки
};

/**
 * @brief Результат операции аутентификации.
 */
struct LoginResult final
{
    bool success { false };
    std::string accessToken;
    std::string tokenType { "Bearer" };
    int expiresIn { 0 };
    ApiError error;
};

/**
 * @brief Результат выхода из системы.
 */
struct LogoutResult final
{
    bool success { false };
    ApiError error;
};

/**
 * @brief HTTP-клиент для REST API «Биом».
 *
 * Содержит только базовые методы; для остальных эндпоинтов методы
 * будут добавляться по мере реализации соответствующих экранов.
 */
class ApiClient final
{
public:
    explicit ApiClient(const std::string& baseUrl);

    void setToken(const std::string& token) { m_token = token; }
    void clearToken() { m_token.clear(); }

    /// Асинхронный вход в систему.
    std::future<LoginResult> login(
        const std::string& login,
        const std::string& password
    );

    /// Асинхронный выход из системы (аннулирование токена).
    std::future<LogoutResult> logout();

private:
    web::http::client::http_client m_client;
    std::string m_token;
};

} // namespace terminal::api