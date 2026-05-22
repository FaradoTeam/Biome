#include "common/log/log.h"

#include "user_team_role_service.h"

namespace server::services
{

UserTeamRoleService::UserTeamRoleService(
    std::shared_ptr<repositories::IUserTeamRoleRepository> utrRepo,
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo
)
    : m_utrRepo(std::move(utrRepo))
    , m_userRepo(std::move(userRepo))
    , m_teamRepo(std::move(teamRepo))
    , m_roleRepo(std::move(roleRepo))
{
    if (!m_utrRepo || !m_userRepo || !m_teamRepo || !m_roleRepo)
    {
        throw std::runtime_error("UserTeamRoleService: repositories are null");
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
        LOG_WARN << "createUserTeamRole: userId, teamId, roleId are required";
        return std::nullopt;
    }

    if (!m_userRepo->findById(*utr.userId).has_value())
    {
        LOG_WARN << "createUserTeamRole: user not found";
        return std::nullopt;
    }
    if (!m_teamRepo->exists(*utr.teamId))
    {
        LOG_WARN << "createUserTeamRole: team not found";
        return std::nullopt;
    }
    if (!m_roleRepo->exists(*utr.roleId))
    {
        LOG_WARN << "createUserTeamRole: role not found";
        return std::nullopt;
    }
    if (m_utrRepo->exists(*utr.userId, *utr.teamId))
    {
        LOG_WARN << "createUserTeamRole: user already has a role in this team";
        return std::nullopt;
    }

    int64_t newId = m_utrRepo->create(utr);
    if (newId <= 0)
        return std::nullopt;

    LOG_INFO << "UserTeamRole created: id=" << newId;
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
        return std::nullopt;

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
    if (utr.userId.has_value() && *utr.userId != *existing->userId)
        needCheck = true;
    if (utr.teamId.has_value() && *utr.teamId != *existing->teamId)
        needCheck = true;
    if (needCheck)
    {
        int64_t newUserId = utr.userId.has_value() ? *utr.userId : *existing->userId;
        int64_t newTeamId = utr.teamId.has_value() ? *utr.teamId : *existing->teamId;
        if (m_utrRepo->exists(newUserId, newTeamId))
        {
            LOG_WARN << "updateUserTeamRole: pair (userId,teamId) already exists";
            return std::nullopt;
        }
    }

    if (!m_utrRepo->update(utr))
        return std::nullopt;
    return m_utrRepo->findById(*utr.id);
}

bool UserTeamRoleService::deleteUserTeamRole(int64_t id)
{
    if (!m_utrRepo->findById(id).has_value())
        return false;
    if (!m_utrRepo->remove(id))
        return false;
    LOG_INFO << "UserTeamRole deleted: id=" << id;
    return true;
}

} // namespace server::services
