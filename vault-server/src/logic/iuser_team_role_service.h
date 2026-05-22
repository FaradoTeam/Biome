#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_team_role.h"

namespace server::services
{

struct UserTeamRolesPage
{
    std::vector<dto::UserTeamRole> items;
    int64_t totalCount = 0;
};

class IUserTeamRoleService
{
public:
    virtual ~IUserTeamRoleService() = default;

    virtual UserTeamRolesPage getUserTeamRoles(
        int page, int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    virtual std::optional<dto::UserTeamRole> getUserTeamRole(int64_t id) = 0;
    virtual std::optional<dto::UserTeamRole> createUserTeamRole(const dto::UserTeamRole& utr) = 0;
    virtual std::optional<dto::UserTeamRole> updateUserTeamRole(const dto::UserTeamRole& utr) = 0;
    virtual bool deleteUserTeamRole(int64_t id) = 0;
};

} // namespace server::services
