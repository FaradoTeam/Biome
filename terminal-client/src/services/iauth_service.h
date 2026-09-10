#pragma once

#include <future>
#include <string>

namespace terminal::services
{

/**
 * @brief Результат операции аутентификации на уровне сервиса.
 */
struct AuthCallResult final
{
    bool success { false };
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса аутентификации.
 */
class IAuthService
{
public:
    virtual ~IAuthService() = default;

    virtual std::future<AuthCallResult> login(
        const std::string& login,
        const std::string& password
    ) = 0;

    virtual std::future<AuthCallResult> logout() = 0;
};

} // namespace terminal::services