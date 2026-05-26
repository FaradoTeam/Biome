#pragma once

#include <memory>

#include "../role_menu_item_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRoleMenuItemRepository final : public IRoleMenuItemRepository
{
public:
    explicit SqliteRoleMenuItemRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRoleMenuItemRepository() override = default;

    std::pair<std::vector<dto::RoleMenuItem>, int64_t> findAll(
        int page, int pageSize, std::optional<int64_t> roleId = std::nullopt
    ) override;
    std::optional<dto::RoleMenuItem> findById(int64_t id) override;
    std::vector<dto::RoleMenuItem> findByRoleId(int64_t roleId) override;
    int64_t create(const dto::RoleMenuItem& menuItem) override;
    bool update(const dto::RoleMenuItem& menuItem) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
