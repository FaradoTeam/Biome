#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/role_menu_item.h"

#include "logic/irole_menu_item_service.h"

namespace server::tests
{

class MockRoleMenuItemService : public services::IRoleMenuItemService
{
public:
    using RoleMenuItemsPage = services::RoleMenuItemsPage;

    void setGetRoleMenuItemsResult(const RoleMenuItemsPage& result)
    {
        m_getRoleMenuItemsResult = result;
    }

    void setGetRoleMenuItemResult(std::optional<dto::RoleMenuItem> item)
    {
        m_getRoleMenuItemResult = std::move(item);
    }

    void setCreateRoleMenuItemResult(std::optional<dto::RoleMenuItem> item)
    {
        m_createRoleMenuItemResult = std::move(item);
    }

    void setUpdateRoleMenuItemResult(std::optional<dto::RoleMenuItem> item)
    {
        m_updateRoleMenuItemResult = std::move(item);
    }

    void setDeleteRoleMenuItemResult(bool result)
    {
        m_deleteRoleMenuItemResult = result;
    }

    // Реализация интерфейса
    RoleMenuItemsPage getRoleMenuItems(int page, int pageSize, std::optional<int64_t> roleId = std::nullopt) override
    {
        m_lastGetRoleMenuItemsPage = page;
        m_lastGetRoleMenuItemsPageSize = pageSize;
        m_lastGetRoleMenuItemsRoleId = roleId;
        ++m_getRoleMenuItemsCallCount;
        return m_getRoleMenuItemsResult;
    }

    std::optional<dto::RoleMenuItem> getRoleMenuItem(int64_t id) override
    {
        m_lastGetRoleMenuItemId = id;
        ++m_getRoleMenuItemCallCount;
        return m_getRoleMenuItemResult;
    }

    std::optional<dto::RoleMenuItem> createRoleMenuItem(const dto::RoleMenuItem& item) override
    {
        m_lastCreatedRoleMenuItem = item;
        ++m_createRoleMenuItemCallCount;
        return m_createRoleMenuItemResult;
    }

    std::optional<dto::RoleMenuItem> updateRoleMenuItem(const dto::RoleMenuItem& item) override
    {
        m_lastUpdatedRoleMenuItem = item;
        ++m_updateRoleMenuItemCallCount;
        return m_updateRoleMenuItemResult;
    }

    bool deleteRoleMenuItem(int64_t id) override
    {
        m_lastDeletedRoleMenuItemId = id;
        ++m_deleteRoleMenuItemCallCount;
        return m_deleteRoleMenuItemResult;
    }

    // Методы для проверки
    int getGetRoleMenuItemsCallCount() const { return m_getRoleMenuItemsCallCount; }
    int getGetRoleMenuItemCallCount() const { return m_getRoleMenuItemCallCount; }
    int getCreateRoleMenuItemCallCount() const { return m_createRoleMenuItemCallCount; }
    int getUpdateRoleMenuItemCallCount() const { return m_updateRoleMenuItemCallCount; }
    int getDeleteRoleMenuItemCallCount() const { return m_deleteRoleMenuItemCallCount; }

    int getLastGetRoleMenuItemsPage() const { return m_lastGetRoleMenuItemsPage; }
    int getLastGetRoleMenuItemsPageSize() const { return m_lastGetRoleMenuItemsPageSize; }
    std::optional<int64_t> getLastGetRoleMenuItemsRoleId() const { return m_lastGetRoleMenuItemsRoleId; }
    int64_t getLastGetRoleMenuItemId() const { return m_lastGetRoleMenuItemId; }
    const dto::RoleMenuItem& getLastCreatedRoleMenuItem() const { return m_lastCreatedRoleMenuItem; }
    const dto::RoleMenuItem& getLastUpdatedRoleMenuItem() const { return m_lastUpdatedRoleMenuItem; }
    int64_t getLastDeletedRoleMenuItemId() const { return m_lastDeletedRoleMenuItemId; }

    void reset()
    {
        m_getRoleMenuItemsCallCount = 0;
        m_getRoleMenuItemCallCount = 0;
        m_createRoleMenuItemCallCount = 0;
        m_updateRoleMenuItemCallCount = 0;
        m_deleteRoleMenuItemCallCount = 0;
        m_lastGetRoleMenuItemsPage = 0;
        m_lastGetRoleMenuItemsPageSize = 0;
        m_lastGetRoleMenuItemsRoleId.reset();
        m_lastGetRoleMenuItemId = 0;
        m_lastCreatedRoleMenuItem = dto::RoleMenuItem {};
        m_lastUpdatedRoleMenuItem = dto::RoleMenuItem {};
        m_lastDeletedRoleMenuItemId = 0;
    }

private:
    RoleMenuItemsPage m_getRoleMenuItemsResult;
    std::optional<dto::RoleMenuItem> m_getRoleMenuItemResult;
    std::optional<dto::RoleMenuItem> m_createRoleMenuItemResult;
    std::optional<dto::RoleMenuItem> m_updateRoleMenuItemResult;
    bool m_deleteRoleMenuItemResult = false;

    int m_getRoleMenuItemsCallCount = 0;
    int m_getRoleMenuItemCallCount = 0;
    int m_createRoleMenuItemCallCount = 0;
    int m_updateRoleMenuItemCallCount = 0;
    int m_deleteRoleMenuItemCallCount = 0;

    int m_lastGetRoleMenuItemsPage = 0;
    int m_lastGetRoleMenuItemsPageSize = 0;
    std::optional<int64_t> m_lastGetRoleMenuItemsRoleId;
    int64_t m_lastGetRoleMenuItemId = 0;
    dto::RoleMenuItem m_lastCreatedRoleMenuItem;
    dto::RoleMenuItem m_lastUpdatedRoleMenuItem;
    int64_t m_lastDeletedRoleMenuItemId = 0;
};

} // namespace server::tests
