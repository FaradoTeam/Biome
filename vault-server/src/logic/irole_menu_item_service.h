#pragma once

#include <optional>
#include <vector>

#include "common/dto/role_menu_item.h"

namespace server::services
{

struct RoleMenuItemsPage
{
    std::vector<dto::RoleMenuItem> items;
    int64_t totalCount = 0;
};

class IRoleMenuItemService
{
public:
    virtual ~IRoleMenuItemService() = default;

    virtual RoleMenuItemsPage getRoleMenuItems(
        int page, int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RoleMenuItem> getRoleMenuItem(int64_t id) = 0;
    virtual std::optional<dto::RoleMenuItem> createRoleMenuItem(const dto::RoleMenuItem& item) = 0;
    virtual std::optional<dto::RoleMenuItem> updateRoleMenuItem(const dto::RoleMenuItem& item) = 0;
    virtual bool deleteRoleMenuItem(int64_t id) = 0;
};

} // namespace server::services
