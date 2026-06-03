#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_team_role_service.h"

#include "repo/role_repository.h"
#include "repo/team_repository.h"
#include "repo/user_repository.h"
#include "repo/user_team_role_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для управления назначениями пользователей в команды.
 */
class UserTeamRoleService final : public IUserTeamRoleService
{
public:
    /**
     * @brief Конструктор.
     * @param utrRepo Репозиторий назначений
     * @param userRepo Репозиторий пользователей
     * @param teamRepo Репозиторий команд
     * @param roleRepo Репозиторий ролей
     * @param authzService Сервис авторизации для проверки прав
     */
    UserTeamRoleService(
        std::shared_ptr<repositories::IUserTeamRoleRepository> utrRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<repositories::IRoleRepository> roleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserTeamRoleService
    UserTeamRolesPage getUserTeamRoles(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) override;

    std::optional<dto::UserTeamRole> getUserTeamRole(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::UserTeamRole> createUserTeamRole(
        const dto::UserTeamRole& userTeamRole,
        int64_t userId
    ) override;

    std::optional<dto::UserTeamRole> updateUserTeamRole(
        const dto::UserTeamRole& userTeamRole,
        int64_t userId
    ) override;

    bool deleteUserTeamRole(
        int64_t id,
        int64_t userId
    ) override;

private:
    void invalidateUserCache(int64_t userId);

private:
    std::shared_ptr<repositories::IUserTeamRoleRepository> m_utrRepo;
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<repositories::IRoleRepository> m_roleRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
