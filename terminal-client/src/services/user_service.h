#pragma once

#include <memory>

#include "../api/api_client.h"

#include "iuser_service.h"

namespace terminal::services
{

class UserService final : public IUserService
{
public:
    explicit UserService(std::shared_ptr<api::ApiClient> api);

    std::future<UsersCallResult> getUsers(
        const models::UserQuery& query
    ) override;

private:
    std::shared_ptr<api::ApiClient> m_api;
};

} // namespace terminal::services
