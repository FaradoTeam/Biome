#include "common/log/log.h"

#include "role_menu_item_service.h"

namespace server::services
{

RoleMenuItemService::RoleMenuItemService(
    std::shared_ptr<repositories::IRoleMenuItemRepository> menuItemRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo
)
    : m_menuItemRepo(std::move(menuItemRepo))
    , m_roleRepo(std::move(roleRepo))
{
    if (!m_menuItemRepo || !m_roleRepo)
    {
        throw std::runtime_error("RoleMenuItemService: repositories are null");
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

std::optional<dto::RoleMenuItem> RoleMenuItemService::createRoleMenuItem(const dto::RoleMenuItem& item)
{
    if (!item.roleId.has_value() || !item.caption.has_value() || item.caption->empty() || !item.link.has_value() || item.link->empty())
    {
        LOG_WARN << "createRoleMenuItem: roleId, caption, link are required";
        return std::nullopt;
    }

    if (!m_roleRepo->exists(*item.roleId))
    {
        LOG_WARN << "createRoleMenuItem: role not found, roleId=" << *item.roleId;
        return std::nullopt;
    }

    int64_t newId = m_menuItemRepo->create(item);
    if (newId <= 0)
        return std::nullopt;

    LOG_INFO << "RoleMenuItem created: id=" << newId;
    return m_menuItemRepo->findById(newId);
}

std::optional<dto::RoleMenuItem> RoleMenuItemService::updateRoleMenuItem(const dto::RoleMenuItem& item)
{
    if (!item.id.has_value())
    {
        LOG_WARN << "updateRoleMenuItem: missing id";
        return std::nullopt;
    }

    auto existing = m_menuItemRepo->findById(*item.id);
    if (!existing)
        return std::nullopt;

    // Если меняется roleId, проверяем существование
    if (item.roleId.has_value() && !m_roleRepo->exists(*item.roleId))
    {
        LOG_WARN << "updateRoleMenuItem: new roleId not found";
        return std::nullopt;
    }

    if (!m_menuItemRepo->update(item))
        return std::nullopt;
    return m_menuItemRepo->findById(*item.id);
}

bool RoleMenuItemService::deleteRoleMenuItem(int64_t id)
{
    if (!m_menuItemRepo->findById(id).has_value())
        return false;
    if (!m_menuItemRepo->remove(id))
        return false;
    LOG_INFO << "RoleMenuItem deleted: id=" << id;
    return true;
}

} // namespace server::services
