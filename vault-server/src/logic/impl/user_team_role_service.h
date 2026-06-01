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

class UserTeamRoleService final : public IUserTeamRoleService
{
public:
    UserTeamRoleService(
        std::shared_ptr<repositories::IUserTeamRoleRepository> utrRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<repositories::IRoleRepository> roleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    UserTeamRolesPage getUserTeamRoles(
        int page, int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) override;

    std::optional<dto::UserTeamRole> getUserTeamRole(int64_t id) override;
    std::optional<dto::UserTeamRole> createUserTeamRole(const dto::UserTeamRole& utr) override;
    std::optional<dto::UserTeamRole> updateUserTeamRole(const dto::UserTeamRole& utr) override;
    bool deleteUserTeamRole(int64_t id) override;

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
