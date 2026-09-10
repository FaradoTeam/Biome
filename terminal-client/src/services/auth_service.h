#pragma once

#include <memory>

#include "iauth_service.h"

#include "../api/api_client.h"
#include "../app/app_state.h"

namespace terminal::services
{

/**
 * @brief Сервис аутентификации.
 *
 * Обёртка над ApiClient, которая синхронизирует AppState и токен
 * внутри ApiClient.
 */
class AuthService final : public IAuthService
{
public:
    AuthService(
        std::shared_ptr<api::ApiClient> api,
        std::shared_ptr<AppState> state
    );

    std::future<AuthCallResult> login(
        const std::string& login,
        const std::string& password
    ) override;

    std::future<AuthCallResult> logout() override;

private:
    std::shared_ptr<api::ApiClient> m_api;
    std::shared_ptr<AppState> m_state;
};

} // namespace terminal::services
