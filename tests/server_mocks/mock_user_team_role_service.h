#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/user_team_role.h"

#include "logic/iuser_team_role_service.h"

namespace server::tests
{

class MockUserTeamRoleService : public services::IUserTeamRoleService
{
public:
    using UserTeamRolesPage = services::UserTeamRolesPage;

    void setGetUserTeamRolesResult(const UserTeamRolesPage& result)
    {
        m_getUserTeamRolesResult = result;
    }

    void setGetUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_getUserTeamRoleResult = std::move(utr);
    }

    void setCreateUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_createUserTeamRoleResult = std::move(utr);
    }

    void setUpdateUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_updateUserTeamRoleResult = std::move(utr);
    }

    void setDeleteUserTeamRoleResult(bool result)
    {
        m_deleteUserTeamRoleResult = result;
    }

    // Реализация интерфейса
    UserTeamRolesPage getUserTeamRoles(int page, int pageSize, std::optional<int64_t> userId = std::nullopt, std::optional<int64_t> teamId = std::nullopt, std::optional<int64_t> roleId = std::nullopt) override
    {
        m_lastGetUserTeamRolesPage = page;
        m_lastGetUserTeamRolesPageSize = pageSize;
        m_lastGetUserTeamRolesUserId = userId;
        m_lastGetUserTeamRolesTeamId = teamId;
        m_lastGetUserTeamRolesRoleId = roleId;
        ++m_getUserTeamRolesCallCount;
        return m_getUserTeamRolesResult;
    }

    std::optional<dto::UserTeamRole> getUserTeamRole(int64_t id) override
    {
        m_lastGetUserTeamRoleId = id;
        ++m_getUserTeamRoleCallCount;
        return m_getUserTeamRoleResult;
    }

    std::optional<dto::UserTeamRole> createUserTeamRole(const dto::UserTeamRole& utr) override
    {
        m_lastCreatedUserTeamRole = utr;
        ++m_createUserTeamRoleCallCount;
        return m_createUserTeamRoleResult;
    }

    std::optional<dto::UserTeamRole> updateUserTeamRole(const dto::UserTeamRole& utr) override
    {
        m_lastUpdatedUserTeamRole = utr;
        ++m_updateUserTeamRoleCallCount;
        return m_updateUserTeamRoleResult;
    }

    bool deleteUserTeamRole(int64_t id) override
    {
        m_lastDeletedUserTeamRoleId = id;
        ++m_deleteUserTeamRoleCallCount;
        return m_deleteUserTeamRoleResult;
    }

    // Методы для проверки
    int getGetUserTeamRolesCallCount() const { return m_getUserTeamRolesCallCount; }
    int getGetUserTeamRoleCallCount() const { return m_getUserTeamRoleCallCount; }
    int getCreateUserTeamRoleCallCount() const { return m_createUserTeamRoleCallCount; }
    int getUpdateUserTeamRoleCallCount() const { return m_updateUserTeamRoleCallCount; }
    int getDeleteUserTeamRoleCallCount() const { return m_deleteUserTeamRoleCallCount; }

    int getLastGetUserTeamRolesPage() const { return m_lastGetUserTeamRolesPage; }
    int getLastGetUserTeamRolesPageSize() const { return m_lastGetUserTeamRolesPageSize; }
    std::optional<int64_t> getLastGetUserTeamRolesUserId() const { return m_lastGetUserTeamRolesUserId; }
    std::optional<int64_t> getLastGetUserTeamRolesTeamId() const { return m_lastGetUserTeamRolesTeamId; }
    std::optional<int64_t> getLastGetUserTeamRolesRoleId() const { return m_lastGetUserTeamRolesRoleId; }
    int64_t getLastGetUserTeamRoleId() const { return m_lastGetUserTeamRoleId; }
    const dto::UserTeamRole& getLastCreatedUserTeamRole() const { return m_lastCreatedUserTeamRole; }
    const dto::UserTeamRole& getLastUpdatedUserTeamRole() const { return m_lastUpdatedUserTeamRole; }
    int64_t getLastDeletedUserTeamRoleId() const { return m_lastDeletedUserTeamRoleId; }

    void reset()
    {
        m_getUserTeamRolesCallCount = 0;
        m_getUserTeamRoleCallCount = 0;
        m_createUserTeamRoleCallCount = 0;
        m_updateUserTeamRoleCallCount = 0;
        m_deleteUserTeamRoleCallCount = 0;
        m_lastGetUserTeamRolesPage = 0;
        m_lastGetUserTeamRolesPageSize = 0;
        m_lastGetUserTeamRolesUserId.reset();
        m_lastGetUserTeamRolesTeamId.reset();
        m_lastGetUserTeamRolesRoleId.reset();
        m_lastGetUserTeamRoleId = 0;
        m_lastCreatedUserTeamRole = dto::UserTeamRole {};
        m_lastUpdatedUserTeamRole = dto::UserTeamRole {};
        m_lastDeletedUserTeamRoleId = 0;
    }

private:
    UserTeamRolesPage m_getUserTeamRolesResult;
    std::optional<dto::UserTeamRole> m_getUserTeamRoleResult;
    std::optional<dto::UserTeamRole> m_createUserTeamRoleResult;
    std::optional<dto::UserTeamRole> m_updateUserTeamRoleResult;
    bool m_deleteUserTeamRoleResult = false;

    int m_getUserTeamRolesCallCount = 0;
    int m_getUserTeamRoleCallCount = 0;
    int m_createUserTeamRoleCallCount = 0;
    int m_updateUserTeamRoleCallCount = 0;
    int m_deleteUserTeamRoleCallCount = 0;

    int m_lastGetUserTeamRolesPage = 0;
    int m_lastGetUserTeamRolesPageSize = 0;
    std::optional<int64_t> m_lastGetUserTeamRolesUserId;
    std::optional<int64_t> m_lastGetUserTeamRolesTeamId;
    std::optional<int64_t> m_lastGetUserTeamRolesRoleId;
    int64_t m_lastGetUserTeamRoleId = 0;
    dto::UserTeamRole m_lastCreatedUserTeamRole;
    dto::UserTeamRole m_lastUpdatedUserTeamRole;
    int64_t m_lastDeletedUserTeamRoleId = 0;
};

} // namespace server::tests
