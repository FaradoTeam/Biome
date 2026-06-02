#include "common/log/log.h"

#include "role_menu_item_service.h"

namespace server::services
{

RoleMenuItemService::RoleMenuItemService(
    std::shared_ptr<repositories::IRoleMenuItemRepository> menuItemRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_menuItemRepo(std::move(menuItemRepo))
    , m_roleRepo(std::move(roleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_menuItemRepo || !m_roleRepo)
    {
        throw std::runtime_error("RoleMenuItemService: репозитории не инициализированы");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("RoleMenuItemService: сервис авторизации не инициализирован");
    }
}

RoleMenuItemsPage RoleMenuItemService::getRoleMenuItems(
    int page, int pageSize, std::optional<int64_t> roleId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_menuItemRepo->findAll(page, pageSize, roleId);
    return { items, total };
}

std::optional<dto::RoleMenuItem> RoleMenuItemService::getRoleMenuItem(int64_t id)
{
    return m_menuItemRepo->findById(id);
}

std::optional<dto::RoleMenuItem> RoleMenuItemService::createRoleMenuItem(
    const dto::RoleMenuItem& item,
    int64_t userId
)
{
    // Только супер-админ может создавать пункты меню для ролей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createRoleMenuItem: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!item.roleId.has_value()
        || !item.caption.has_value()
        || item.caption->empty()
        || !item.link.has_value()
        || item.link->empty())
    {
        LOG_WARN << "createRoleMenuItem: обязательны roleId, caption и link";
        return std::nullopt;
    }

    if (!m_roleRepo->exists(*item.roleId))
    {
        LOG_WARN << "createRoleMenuItem: роль не найдена, roleId=" << *item.roleId;
        return std::nullopt;
    }

    int64_t newId = m_menuItemRepo->create(item);
    if (newId <= 0)
        return std::nullopt;

    LOG_INFO << "Пункт меню роли создан: id=" << newId << ", пользователь=" << userId;

    // Инвалидируем кэш для пользователей с этой ролью
    invalidateUsersByRoleId(*item.roleId);

    return m_menuItemRepo->findById(newId);
}

std::optional<dto::RoleMenuItem> RoleMenuItemService::updateRoleMenuItem(
    const dto::RoleMenuItem& item,
    int64_t userId
)
{
    // Только супер-админ может обновлять пункты меню для ролей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateRoleMenuItem: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!item.id.has_value())
    {
        LOG_WARN << "updateRoleMenuItem: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_menuItemRepo->findById(*item.id);
    if (!existing)
    {
        LOG_WARN << "updateRoleMenuItem: пункт меню не найден, id=" << *item.id;
        return std::nullopt;
    }

    // Если меняется roleId, проверяем существование новой роли
    int64_t oldRoleId = *existing->roleId;
    if (item.roleId.has_value() && *item.roleId != oldRoleId)
    {
        if (!m_roleRepo->exists(*item.roleId))
        {
            LOG_WARN << "updateRoleMenuItem: новая roleId не найдена, roleId=" << *item.roleId;
            return std::nullopt;
        }
    }

    if (!m_menuItemRepo->update(item))
    {
        LOG_ERROR << "updateRoleMenuItem: не удалось обновить пункт меню id=" << *item.id;
        return std::nullopt;
    }

    LOG_INFO << "Пункт меню роли обновлен: id=" << *item.id << ", пользователь=" << userId;

    // Инвалидируем кэш по старой и новой роли
    invalidateUsersByRoleId(oldRoleId);
    if (item.roleId.has_value() && *item.roleId != oldRoleId)
    {
        invalidateUsersByRoleId(*item.roleId);
    }

    return m_menuItemRepo->findById(*item.id);
}

bool RoleMenuItemService::deleteRoleMenuItem(int64_t id, int64_t userId)
{
    // Только супер-админ может удалять пункты меню для ролей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteRoleMenuItem: пользователь " << userId << " не имеет прав";
        return false;
    }

    auto existing = m_menuItemRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRoleMenuItem: пункт меню не найден, id=" << id;
        return false;
    }

    int64_t roleId = *existing->roleId;

    if (!m_menuItemRepo->remove(id))
    {
        LOG_ERROR << "deleteRoleMenuItem: не удалось удалить пункт меню id=" << id;
        return false;
    }

    LOG_INFO << "Пункт меню роли удален: id=" << id << ", пользователь=" << userId;

    // Инвалидируем кэш для пользователей с этой ролью
    invalidateUsersByRoleId(roleId);

    return true;
}

void RoleMenuItemService::invalidateUsersByRoleId(int64_t roleId)
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
