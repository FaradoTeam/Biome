#include "common/log/log.h"

#include "auth_service.h"

namespace terminal::services
{

AuthService::AuthService(
    std::shared_ptr<api::ApiClient> api,
    std::shared_ptr<AppState> state
)
    : m_api(std::move(api))
    , m_state(std::move(state))
{
    if (!m_api || !m_state)
    {
        throw std::runtime_error("AuthService: зависимости не инициализированы");
    }
}

std::future<AuthCallResult> AuthService::login(
    const std::string& login,
    const std::string& password
)
{
    return std::async(
        std::launch::async,
        [this, login, password]() -> AuthCallResult
        {
            AuthCallResult result;

            auto apiResult = m_api->login(login, password).get();
            result.success = apiResult.success;

            if (apiResult.success)
            {
                m_state->setLoggedIn(true);
                m_state->setToken(apiResult.accessToken);
                m_api->setToken(apiResult.accessToken);
                LOG_INFO << "AuthService: вход выполнен";
            }
            else
            {
                result.errorMessage = apiResult.error.message;
                LOG_WARN
                    << "AuthService: вход не выполнен — "
                    << apiResult.error.message;
            }

            return result;
        }
    );
}

std::future<AuthCallResult> AuthService::logout()
{
    return std::async(
        std::launch::async,
        [this]() -> AuthCallResult
        {
            AuthCallResult result;

            auto apiResult = m_api->logout().get();
            result.success = apiResult.success;

            if (!apiResult.success)
            {
                result.errorMessage = apiResult.error.message;
                LOG_WARN
                    << "AuthService: logout на сервере не удался — "
                    << apiResult.error.message;
            }

            // Локальное состояние очищаем в любом случае.
            m_state->logout();
            m_api->clearToken();
            LOG_INFO << "AuthService: локальное состояние сброшено";

            return result;
        }
    );
}

} // namespace terminal::services
