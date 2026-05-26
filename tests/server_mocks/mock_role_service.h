#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/role.h"

#include "logic/irole_service.h"

namespace server::tests
{

class MockRoleService : public services::IRoleService
{
public:
    using RolesPage = services::RolesPage;

    void setGetRolesResult(const RolesPage& result)
    {
        m_getRolesResult = result;
    }

    void setGetRoleResult(std::optional<dto::Role> role)
    {
        m_getRoleResult = std::move(role);
    }

    void setCreateRoleResult(std::optional<dto::Role> role)
    {
        m_createRoleResult = std::move(role);
    }

    void setUpdateRoleResult(std::optional<dto::Role> role)
    {
        m_updateRoleResult = std::move(role);
    }

    void setDeleteRoleResult(bool result)
    {
        m_deleteRoleResult = result;
    }

    // Реализация интерфейса
    RolesPage getRoles(int page, int pageSize, const std::string& searchCaption = "") override
    {
        m_lastGetRolesPage = page;
        m_lastGetRolesPageSize = pageSize;
        m_lastGetRolesSearch = searchCaption;
        ++m_getRolesCallCount;
        return m_getRolesResult;
    }

    std::optional<dto::Role> getRole(int64_t id) override
    {
        m_lastGetRoleId = id;
        ++m_getRoleCallCount;
        return m_getRoleResult;
    }

    std::optional<dto::Role> createRole(const dto::Role& role) override
    {
        m_lastCreatedRole = role;
        ++m_createRoleCallCount;
        return m_createRoleResult;
    }

    std::optional<dto::Role> updateRole(const dto::Role& role) override
    {
        m_lastUpdatedRole = role;
        ++m_updateRoleCallCount;
        return m_updateRoleResult;
    }

    bool deleteRole(int64_t id) override
    {
        m_lastDeletedRoleId = id;
        ++m_deleteRoleCallCount;
        return m_deleteRoleResult;
    }

    // Методы для проверки вызовов
    int getGetRolesCallCount() const { return m_getRolesCallCount; }
    int getGetRoleCallCount() const { return m_getRoleCallCount; }
    int getCreateRoleCallCount() const { return m_createRoleCallCount; }
    int getUpdateRoleCallCount() const { return m_updateRoleCallCount; }
    int getDeleteRoleCallCount() const { return m_deleteRoleCallCount; }

    int getLastGetRolesPage() const { return m_lastGetRolesPage; }
    int getLastGetRolesPageSize() const { return m_lastGetRolesPageSize; }
    const std::string& getLastGetRolesSearch() const { return m_lastGetRolesSearch; }
    int64_t getLastGetRoleId() const { return m_lastGetRoleId; }
    const dto::Role& getLastCreatedRole() const { return m_lastCreatedRole; }
    const dto::Role& getLastUpdatedRole() const { return m_lastUpdatedRole; }
    int64_t getLastDeletedRoleId() const { return m_lastDeletedRoleId; }

    void reset()
    {
        m_getRolesCallCount = 0;
        m_getRoleCallCount = 0;
        m_createRoleCallCount = 0;
        m_updateRoleCallCount = 0;
        m_deleteRoleCallCount = 0;
        m_lastGetRolesPage = 0;
        m_lastGetRolesPageSize = 0;
        m_lastGetRolesSearch.clear();
        m_lastGetRoleId = 0;
        m_lastCreatedRole = dto::Role {};
        m_lastUpdatedRole = dto::Role {};
        m_lastDeletedRoleId = 0;
    }

private:
    RolesPage m_getRolesResult;
    std::optional<dto::Role> m_getRoleResult;
    std::optional<dto::Role> m_createRoleResult;
    std::optional<dto::Role> m_updateRoleResult;
    bool m_deleteRoleResult = false;

    int m_getRolesCallCount = 0;
    int m_getRoleCallCount = 0;
    int m_createRoleCallCount = 0;
    int m_updateRoleCallCount = 0;
    int m_deleteRoleCallCount = 0;

    int m_lastGetRolesPage = 0;
    int m_lastGetRolesPageSize = 0;
    std::string m_lastGetRolesSearch;
    int64_t m_lastGetRoleId = 0;
    dto::Role m_lastCreatedRole;
    dto::Role m_lastUpdatedRole;
    int64_t m_lastDeletedRoleId = 0;
};

} // namespace server::tests
