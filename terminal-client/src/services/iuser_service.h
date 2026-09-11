#pragma once

#include <future>

#include "../api/models/page.h"
#include "../api/models/user.h"
#include "../api/models/user_query.h"

namespace terminal::services
{

/**
 * @brief Результат операции со списком пользователей.
 */
struct UsersCallResult final
{
    bool success { false };
    models::Page<models::User> page;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса пользователей.
 */
class IUserService
{
public:
    virtual ~IUserService() = default;

    virtual std::future<UsersCallResult> getUsers(
        const models::UserQuery& query
    ) = 0;
};

} // namespace terminal::services
