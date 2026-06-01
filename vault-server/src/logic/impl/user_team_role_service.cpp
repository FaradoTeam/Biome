#include "common/log/log.h"

#include "user_team_role_service.h"

namespace server::services
{

UserTeamRoleService::UserTeamRoleService(
    std::shared_ptr<repositories::IUserTeamRoleRepository> utrRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_utrRepo(std::move(utrRepo))
    , m_userRepo(std::move(userRepo))
    , m_teamRepo(std::move(teamRepo))
    , m_roleRepo(std::move(roleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_utrRepo || !m_userRepo || !m_teamRepo || !m_roleRepo)
    {
        throw std::runtime_error("UserTeamRoleService: repositories are null");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserTeamRoleService: authorizationService is null");
    }
}

UserTeamRolesPage UserTeamRoleService::getUserTeamRoles(
    int page, int pageSize,
    std::optional<int64_t> userId,
    std::optional<int64_t> teamId,
    std::optional<int64_t> roleId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_utrRepo->findAll(page, pageSize, userId, teamId, roleId);
    return { items, total };
}

std::optional<dto::UserTeamRole> UserTeamRoleService::getUserTeamRole(int64_t id)
{
    return m_utrRepo->findById(id);
}

std::optional<dto::UserTeamRole> UserTeamRoleService::createUserTeamRole(const dto::UserTeamRole& utr)
{
    if (!utr.userId.has_value() || !utr.teamId.has_value() || !utr.roleId.has_value())
    {
        LOG_WARN << "createUserTeamRole: userId, teamId and roleId are required";
        return std::nullopt;
    }

    // Проверяем существование пользователя
    if (!m_userRepo->findById(*utr.userId).has_value())
    {
        LOG_WARN << "createUserTeamRole: user not found, userId=" << *utr.userId;
        return std::nullopt;
    }

    // Проверяем существование команды
    if (!m_teamRepo->exists(*utr.teamId))
    {
        LOG_WARN << "createUserTeamRole: team not found, teamId=" << *utr.teamId;
        return std::nullopt;
    }

    // Проверяем существование роли
    if (!m_roleRepo->exists(*utr.roleId))
    {
        LOG_WARN << "createUserTeamRole: role not found, roleId=" << *utr.roleId;
        return std::nullopt;
    }

    // Проверяем, что пользователь ещё не имеет роли в этой команде
    if (m_utrRepo->exists(*utr.userId, *utr.teamId))
    {
        LOG_WARN << "createUserTeamRole: user already has a role in this team";
        return std::nullopt;
    }

    const int64_t newId = m_utrRepo->create(utr);
    if (newId <= 0)
    {
        LOG_ERROR << "createUserTeamRole: failed to create";
        return std::nullopt;
    }

    LOG_INFO
        << "UserTeamRole created: id=" << newId
        << ", userId=" << *utr.userId
        << ", teamId=" << *utr.teamId
        << ", roleId=" << *utr.roleId;

    // Инвалидируем кэш для этого пользователя
    invalidateUserCache(*utr.userId);

    return m_utrRepo->findById(newId);
}

std::optional<dto::UserTeamRole> UserTeamRoleService::updateUserTeamRole(const dto::UserTeamRole& utr)
{
    if (!utr.id.has_value())
    {
        LOG_WARN << "updateUserTeamRole: missing id";
        return std::nullopt;
    }

    auto existing = m_utrRepo->findById(*utr.id);
    if (!existing)
    {
        LOG_WARN << "updateUserTeamRole: UserTeamRole not found, id=" << *utr.id;
        return std::nullopt;
    }

    const int64_t userId = *existing->userId;

    // Проверяем корректность новых внешних ключей
    if (utr.userId.has_value() && !m_userRepo->findById(*utr.userId).has_value())
    {
        LOG_WARN << "updateUserTeamRole: new userId not found";
        return std::nullopt;
    }
    if (utr.teamId.has_value() && !m_teamRepo->exists(*utr.teamId))
    {
        LOG_WARN << "updateUserTeamRole: new teamId not found";
        return std::nullopt;
    }
    if (utr.roleId.has_value() && !m_roleRepo->exists(*utr.roleId))
    {
        LOG_WARN << "updateUserTeamRole: new roleId not found";
        return std::nullopt;
    }

    // Если меняется пара (userId, teamId), проверяем уникальность
    bool needCheck = false;
    int64_t newUserId = userId;
    int64_t newTeamId = *existing->teamId;

    if (utr.userId.has_value() && *utr.userId != userId)
    {
        newUserId = *utr.userId;
        needCheck = true;
    }
    if (utr.teamId.has_value() && *utr.teamId != newTeamId)
    {
        newTeamId = *utr.teamId;
        needCheck = true;
    }

    if (needCheck && m_utrRepo->exists(newUserId, newTeamId))
    {
        LOG_WARN << "updateUserTeamRole: pair (userId,teamId) already exists";
        return std::nullopt;
    }

    if (!m_utrRepo->update(utr))
    {
        LOG_ERROR << "updateUserTeamRole: failed to update id=" << *utr.id;
        return std::nullopt;
    }

    LOG_INFO << "UserTeamRole updated: id=" << *utr.id;

    // Инвалидируем кэш для пользователя
    invalidateUserCache(userId);
    if (utr.userId.has_value() && *utr.userId != userId)
    {
        invalidateUserCache(*utr.userId);
    }

    return m_utrRepo->findById(*utr.id);
}

bool UserTeamRoleService::deleteUserTeamRole(int64_t id)
{
    auto existing = m_utrRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteUserTeamRole: UserTeamRole not found, id=" << id;
        return false;
    }

    const int64_t userId = *existing->userId;

    if (!m_utrRepo->remove(id))
    {
        LOG_ERROR << "deleteUserTeamRole: failed to delete id=" << id;
        return false;
    }

    LOG_INFO << "UserTeamRole deleted: id=" << id;

    // Инвалидируем кэш для этого пользователя
    invalidateUserCache(userId);

    return true;
}

void UserTeamRoleService::invalidateUserCache(int64_t userId)
{
    m_authzService->invalidateCache(userId);
    LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
}

} // namespace server::services
