#pragma once

#include <optional>
#include <vector>

#include "common/dto/role_menu_item.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для пунктов меню ролей.
 */
class IRoleMenuItemRepository
{
public:
    virtual ~IRoleMenuItemRepository() = default;

    virtual std::pair<std::vector<dto::RoleMenuItem>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RoleMenuItem> findById(int64_t id) = 0;
    virtual std::vector<dto::RoleMenuItem> findByRoleId(int64_t roleId) = 0;
    virtual int64_t create(const dto::RoleMenuItem& menuItem) = 0;
    virtual bool update(const dto::RoleMenuItem& menuItem) = 0;
    virtual bool remove(int64_t id) = 0;
    virtual bool exists(int64_t id) = 0;
};

} // namespace server::repositories
