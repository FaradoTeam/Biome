#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/irole_menu_item_service.h"

#include "repo/role_menu_item_repository.h"
#include "repo/role_repository.h"

namespace server::services
{

class RoleMenuItemService final : public IRoleMenuItemService
{
public:
    RoleMenuItemService(
        std::shared_ptr<repositories::IRoleMenuItemRepository> menuItemRepo,
        std::shared_ptr<repositories::IRoleRepository> roleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    RoleMenuItemsPage getRoleMenuItems(
        int page, int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) override;

    std::optional<dto::RoleMenuItem> getRoleMenuItem(int64_t id) override;

    std::optional<dto::RoleMenuItem> createRoleMenuItem(
        const dto::RoleMenuItem& item,
        int64_t userId
    ) override;

    std::optional<dto::RoleMenuItem> updateRoleMenuItem(
        const dto::RoleMenuItem& item,
        int64_t userId
    ) override;

    bool deleteRoleMenuItem(
        int64_t id,
        int64_t userId
    ) override;

private:
    void invalidateUsersByRoleId(int64_t roleId);

private:
    std::shared_ptr<repositories::IRoleMenuItemRepository> m_menuItemRepo;
    std::shared_ptr<repositories::IRoleRepository> m_roleRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
