#include "user_service.h"

namespace terminal::services
{

UserService::UserService(std::shared_ptr<api::ApiClient> api)
    : m_api(std::move(api))
{
    if (!m_api)
        throw std::runtime_error("UserService: ApiClient не инициализирован");
}

std::future<UsersCallResult> UserService::getUsers(
    const models::UserQuery& query
)
{
    return std::async(
        std::launch::async,
        [this, query]() -> UsersCallResult
        {
            UsersCallResult result;

            auto apiResult = m_api->getUsers(query).get();
            result.success = apiResult.success;
            result.page = std::move(apiResult.page);

            if (!apiResult.success)
                result.errorMessage = apiResult.error.message;

            return result;
        }
    );
}

} // namespace terminal::services
