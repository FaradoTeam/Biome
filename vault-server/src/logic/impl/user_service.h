#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_service.h"

#include "repo/user_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для управления пользователями.
 */
class UserService final : public IUserService
{
public:
    /**
     * @brief Конструктор.
     * @param userRepo Репозиторий пользователей
     * @param authzService Сервис авторизации для проверки прав
     */
    UserService(
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserService
    UsersPage users(
        int page,
        int pageSize,
        int64_t userId,
        const std::string& login = "",
        const std::string& name = "",
        const std::string& email = "",
        std::optional<bool> isBlocked = std::nullopt
    ) override;

    std::optional<dto::User> user(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::User> createUser(
        const dto::User& user,
        const std::string& password,
        int64_t userId
    ) override;

    std::optional<dto::User> updateUser(
        const dto::User& user,
        int64_t userId
    ) override;

    bool deleteUser(
        int64_t id,
        int64_t userId
    ) override;

private:
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
