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

    MockUserTeamRoleService()
        : m_createUserTeamRoleConflict(false)
    {
    }

    void setGetUserTeamRolesResult(const UserTeamRolesPage& result)
    {
        m_getUserTeamRolesResult = result;
        m_getUserTeamRolesCallback = nullptr;
    }

    void setGetUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_getUserTeamRoleResult = std::move(utr);
        m_getUserTeamRoleCallback = nullptr;
    }

    void setCreateUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_createUserTeamRoleResult = std::move(utr);
        m_createUserTeamRoleCallback = nullptr;
    }

    void setUpdateUserTeamRoleResult(std::optional<dto::UserTeamRole> utr)
    {
        m_updateUserTeamRoleResult = std::move(utr);
        m_updateUserTeamRoleCallback = nullptr;
    }

    void setDeleteUserTeamRoleResult(bool result)
    {
        m_deleteUserTeamRoleResult = result;
        m_deleteUserTeamRoleCallback = nullptr;
    }

    void setCreateUserTeamRoleConflict(bool conflict)
    {
        m_createUserTeamRoleConflict = conflict;
    }

    // Callback-и для кастомной логики
    void setGetUserTeamRolesCallback(
        std::function<UserTeamRolesPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_getUserTeamRolesCallback = std::move(callback);
    }

    void setGetUserTeamRoleCallback(
        std::function<std::optional<dto::UserTeamRole>(int64_t, int64_t)> callback
    )
    {
        m_getUserTeamRoleCallback = std::move(callback);
    }

    void setCreateUserTeamRoleCallback(
        std::function<std::optional<dto::UserTeamRole>(const dto::UserTeamRole&, int64_t)> callback
    )
    {
        m_createUserTeamRoleCallback = std::move(callback);
    }

    void setUpdateUserTeamRoleCallback(
        std::function<std::optional<dto::UserTeamRole>(const dto::UserTeamRole&, int64_t)> callback
    )
    {
        m_updateUserTeamRoleCallback = std::move(callback);
    }

    void setDeleteUserTeamRoleCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteUserTeamRoleCallback = std::move(callback);
    }

    // Реализация интерфейса IUserTeamRoleService
    services::UserTeamRolesPage getUserTeamRoles(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) override
    {
        m_lastGetUserTeamRolesPage = page;
        m_lastGetUserTeamRolesPageSize = pageSize;
        m_lastGetUserTeamRolesRequestUserId = userId;
        m_lastGetUserTeamRolesFilterUserId = filterUserId;
        m_lastGetUserTeamRolesTeamId = teamId;
        m_lastGetUserTeamRolesRoleId = roleId;
        ++m_getUserTeamRolesCallCount;

        if (m_getUserTeamRolesCallback)
        {
            return m_getUserTeamRolesCallback(page, pageSize, userId, filterUserId, teamId, roleId);
        }

        // Только супер-админ (userId=1) может просматривать назначения
        if (userId != 1)
        {
            services::UserTeamRolesPage emptyPage;
            emptyPage.totalCount = 0;
            return emptyPage;
        }

        return m_getUserTeamRolesResult;
    }

    std::optional<dto::UserTeamRole> getUserTeamRole(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastGetUserTeamRoleId = id;
        m_lastGetUserTeamRoleRequestUserId = userId;
        ++m_getUserTeamRoleCallCount;

        if (m_getUserTeamRoleCallback)
        {
            return m_getUserTeamRoleCallback(id, userId);
        }

        // Только супер-админ (userId=1) может просматривать назначение
        if (userId != 1)
        {
            return std::nullopt;
        }

        return m_getUserTeamRoleResult;
    }

    std::optional<dto::UserTeamRole> createUserTeamRole(
        const dto::UserTeamRole& utr,
        int64_t userId
    ) override
    {
        m_lastCreatedUserTeamRole = utr;
        m_lastCreateUserTeamRoleUserId = userId;
        ++m_createUserTeamRoleCallCount;

        if (m_createUserTeamRoleCallback)
        {
            return m_createUserTeamRoleCallback(utr, userId);
        }

        // Только супер-админ (userId=1) может создавать назначения
        if (userId != 1)
        {
            return std::nullopt;
        }

        // Симуляция конфликта (дубликат)
        if (m_createUserTeamRoleConflict)
        {
            return std::nullopt;
        }

        return m_createUserTeamRoleResult;
    }

    std::optional<dto::UserTeamRole> updateUserTeamRole(
        const dto::UserTeamRole& utr,
        int64_t userId
    ) override
    {
        m_lastUpdatedUserTeamRole = utr;
        m_lastUpdateUserTeamRoleUserId = userId;
        ++m_updateUserTeamRoleCallCount;

        if (m_updateUserTeamRoleCallback)
        {
            return m_updateUserTeamRoleCallback(utr, userId);
        }

        // Только супер-админ (userId=1) может обновлять назначения
        if (userId != 1)
        {
            return std::nullopt;
        }

        return m_updateUserTeamRoleResult;
    }

    bool deleteUserTeamRole(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedUserTeamRoleId = id;
        m_lastDeleteUserTeamRoleUserId = userId;
        ++m_deleteUserTeamRoleCallCount;

        if (m_deleteUserTeamRoleCallback)
        {
            return m_deleteUserTeamRoleCallback(id, userId);
        }

        // Только супер-админ (userId=1) может удалять назначения
        if (userId != 1)
        {
            return false;
        }

        return m_deleteUserTeamRoleResult;
    }

    // Методы для проверки вызовов
    int getGetUserTeamRolesCallCount() const { return m_getUserTeamRolesCallCount; }
    int getGetUserTeamRoleCallCount() const { return m_getUserTeamRoleCallCount; }
    int getCreateUserTeamRoleCallCount() const { return m_createUserTeamRoleCallCount; }
    int getUpdateUserTeamRoleCallCount() const { return m_updateUserTeamRoleCallCount; }
    int getDeleteUserTeamRoleCallCount() const { return m_deleteUserTeamRoleCallCount; }

    int getLastGetUserTeamRolesPage() const { return m_lastGetUserTeamRolesPage; }
    int getLastGetUserTeamRolesPageSize() const { return m_lastGetUserTeamRolesPageSize; }
    int64_t getLastGetUserTeamRolesRequestUserId() const { return m_lastGetUserTeamRolesRequestUserId; }
    std::optional<int64_t> getLastGetUserTeamRolesFilterUserId() const { return m_lastGetUserTeamRolesFilterUserId; }
    std::optional<int64_t> getLastGetUserTeamRolesTeamId() const { return m_lastGetUserTeamRolesTeamId; }
    std::optional<int64_t> getLastGetUserTeamRolesRoleId() const { return m_lastGetUserTeamRolesRoleId; }
    int64_t getLastGetUserTeamRoleId() const { return m_lastGetUserTeamRoleId; }
    int64_t getLastGetUserTeamRoleRequestUserId() const { return m_lastGetUserTeamRoleRequestUserId; }
    const dto::UserTeamRole& getLastCreatedUserTeamRole() const { return m_lastCreatedUserTeamRole; }
    int64_t getLastCreateUserTeamRoleUserId() const { return m_lastCreateUserTeamRoleUserId; }
    const dto::UserTeamRole& getLastUpdatedUserTeamRole() const { return m_lastUpdatedUserTeamRole; }
    int64_t getLastUpdateUserTeamRoleUserId() const { return m_lastUpdateUserTeamRoleUserId; }
    int64_t getLastDeletedUserTeamRoleId() const { return m_lastDeletedUserTeamRoleId; }
    int64_t getLastDeleteUserTeamRoleUserId() const { return m_lastDeleteUserTeamRoleUserId; }

    void reset()
    {
        m_getUserTeamRolesCallCount = 0;
        m_getUserTeamRoleCallCount = 0;
        m_createUserTeamRoleCallCount = 0;
        m_updateUserTeamRoleCallCount = 0;
        m_deleteUserTeamRoleCallCount = 0;

        m_lastGetUserTeamRolesPage = 0;
        m_lastGetUserTeamRolesPageSize = 0;
        m_lastGetUserTeamRolesRequestUserId = 0;
        m_lastGetUserTeamRolesFilterUserId.reset();
        m_lastGetUserTeamRolesTeamId.reset();
        m_lastGetUserTeamRolesRoleId.reset();
        m_lastGetUserTeamRoleId = 0;
        m_lastGetUserTeamRoleRequestUserId = 0;
        m_lastCreatedUserTeamRole = dto::UserTeamRole {};
        m_lastCreateUserTeamRoleUserId = 0;
        m_lastUpdatedUserTeamRole = dto::UserTeamRole {};
        m_lastUpdateUserTeamRoleUserId = 0;
        m_lastDeletedUserTeamRoleId = 0;
        m_lastDeleteUserTeamRoleUserId = 0;

        m_getUserTeamRolesCallback = nullptr;
        m_getUserTeamRoleCallback = nullptr;
        m_createUserTeamRoleCallback = nullptr;
        m_updateUserTeamRoleCallback = nullptr;
        m_deleteUserTeamRoleCallback = nullptr;

        m_getUserTeamRolesResult = services::UserTeamRolesPage {};
        m_getUserTeamRoleResult = std::nullopt;
        m_createUserTeamRoleResult = std::nullopt;
        m_updateUserTeamRoleResult = std::nullopt;
        m_deleteUserTeamRoleResult = false;
        m_createUserTeamRoleConflict = false;
        m_nextId = 100;
    }

private:
    services::UserTeamRolesPage m_getUserTeamRolesResult;
    std::optional<dto::UserTeamRole> m_getUserTeamRoleResult;
    std::optional<dto::UserTeamRole> m_createUserTeamRoleResult;
    std::optional<dto::UserTeamRole> m_updateUserTeamRoleResult;
    bool m_deleteUserTeamRoleResult = false;
    bool m_createUserTeamRoleConflict = false;

    // Callback-и
    std::function<services::UserTeamRolesPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> m_getUserTeamRolesCallback;
    std::function<std::optional<dto::UserTeamRole>(int64_t, int64_t)> m_getUserTeamRoleCallback;
    std::function<std::optional<dto::UserTeamRole>(const dto::UserTeamRole&, int64_t)> m_createUserTeamRoleCallback;
    std::function<std::optional<dto::UserTeamRole>(const dto::UserTeamRole&, int64_t)> m_updateUserTeamRoleCallback;
    std::function<bool(int64_t, int64_t)> m_deleteUserTeamRoleCallback;

    // Счётчики вызовов
    int m_getUserTeamRolesCallCount = 0;
    int m_getUserTeamRoleCallCount = 0;
    int m_createUserTeamRoleCallCount = 0;
    int m_updateUserTeamRoleCallCount = 0;
    int m_deleteUserTeamRoleCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetUserTeamRolesPage = 0;
    int m_lastGetUserTeamRolesPageSize = 0;
    int64_t m_lastGetUserTeamRolesRequestUserId = 0;
    std::optional<int64_t> m_lastGetUserTeamRolesFilterUserId;
    std::optional<int64_t> m_lastGetUserTeamRolesTeamId;
    std::optional<int64_t> m_lastGetUserTeamRolesRoleId;
    int64_t m_lastGetUserTeamRoleId = 0;
    int64_t m_lastGetUserTeamRoleRequestUserId = 0;
    dto::UserTeamRole m_lastCreatedUserTeamRole;
    int64_t m_lastCreateUserTeamRoleUserId = 0;
    dto::UserTeamRole m_lastUpdatedUserTeamRole;
    int64_t m_lastUpdateUserTeamRoleUserId = 0;
    int64_t m_lastDeletedUserTeamRoleId = 0;
    int64_t m_lastDeleteUserTeamRoleUserId = 0;
    int64_t m_nextId = 100;
};

} // namespace server::tests
