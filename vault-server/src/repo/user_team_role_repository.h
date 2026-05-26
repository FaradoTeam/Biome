#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_team_role.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для назначений пользователей в команды с ролями.
 */
class IUserTeamRoleRepository
{
public:
    virtual ~IUserTeamRoleRepository() = default;

    virtual std::pair<std::vector<dto::UserTeamRole>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    virtual std::optional<dto::UserTeamRole> findById(int64_t id) = 0;
    virtual std::vector<dto::UserTeamRole> findByUserId(int64_t userId) = 0;
    virtual std::vector<dto::UserTeamRole> findByTeamId(int64_t teamId) = 0;
    virtual std::optional<dto::UserTeamRole> findByUserAndTeam(int64_t userId, int64_t teamId) = 0;
    virtual bool exists(int64_t userId, int64_t teamId) = 0;
    virtual int64_t create(const dto::UserTeamRole& userTeamRole) = 0;
    virtual bool update(const dto::UserTeamRole& userTeamRole) = 0;
    virtual bool remove(int64_t id) = 0;
    virtual bool removeByUserAndTeam(int64_t userId, int64_t teamId) = 0;
};

} // namespace server::repositories
