#include "common/log/log.h"

#include "role_service.h"

namespace server::services
{

RoleService::RoleService(
    std::shared_ptr<repositories::IRoleRepository> roleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_roleRepo(std::move(roleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_roleRepo)
    {
        throw std::runtime_error("RoleService: roleRepository is null");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("RoleService: authorizationService is null");
    }
}

RolesPage RoleService::getRoles(int page, int pageSize, const std::string& searchCaption)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [roles, total] = m_roleRepo->findAll(page, pageSize, searchCaption);
    return { roles, total };
}

std::optional<dto::Role> RoleService::getRole(int64_t id)
{
    return m_roleRepo->findById(id);
}

std::optional<dto::Role> RoleService::createRole(const dto::Role& role)
{
    if (!role.caption.has_value() || role.caption->empty())
    {
        LOG_WARN << "createRole: caption is required";
        return std::nullopt;
    }

    int64_t newId = m_roleRepo->create(role);
    if (newId <= 0)
    {
        LOG_ERROR << "createRole: failed to create role";
        return std::nullopt;
    }

    LOG_INFO << "Role created: id=" << newId << ", caption=" << *role.caption;

    // Роль новая, пользователей с ней ещё нет, поэтому инвалидация не требуется
    return m_roleRepo->findById(newId);
}

std::optional<dto::Role> RoleService::updateRole(const dto::Role& role)
{
    if (!role.id.has_value())
    {
        LOG_WARN << "updateRole: missing id";
        return std::nullopt;
    }

    auto existing = m_roleRepo->findById(*role.id);
    if (!existing)
    {
        LOG_WARN << "updateRole: role not found, id=" << *role.id;
        return std::nullopt;
    }

    if (!m_roleRepo->update(role))
    {
        LOG_ERROR << "updateRole: failed to update role id=" << *role.id;
        return std::nullopt;
    }

    LOG_INFO << "Role updated: id=" << *role.id;

    // Изменение роли может повлиять на права (например, изменилось название - не влияет)
    // Но для консистентности инвалидируем кэш пользователей с этой ролью
    // Если caption не меняется, можно пропустить, но для простоты инвалидируем всегда
    invalidateUsersByRoleId(*role.id);

    return m_roleRepo->findById(*role.id);
}

bool RoleService::deleteRole(int64_t id)
{
    auto existing = m_roleRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRole: role not found, id=" << id;
        return false;
    }

    // TODO: проверить, что роль не используется в UserTeamRole, Rule и других таблицах
    // Это можно сделать через соответствующие репозитории

    if (!m_roleRepo->remove(id))
    {
        LOG_ERROR << "deleteRole: failed to delete role id=" << id;
        return false;
    }

    LOG_INFO << "Role deleted: id=" << id;

    // Инвалидируем кэш для всех пользователей, у которых была эта роль
    invalidateUsersByRoleId(id);

    return true;
}

void RoleService::invalidateUsersByRoleId(int64_t roleId)
{
    auto users = m_authzService->getUserIdsByRoleId(roleId);
    LOG_DEBUG << "Инвалидация кэша для " << users.size() << " пользователей с ролью " << roleId;

    for (int64_t userId : users)
    {
        m_authzService->invalidateCache(userId);
        LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
    }
}

} // namespace server::services
