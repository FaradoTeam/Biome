#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/user.h"

#include "logic/iuser_service.h"

namespace server
{
namespace tests
{

class MockUserService : public services::IUserService
{
public:
    using UsersPage = services::UsersPage;

    // Настройка результатов
    void setGetUsersResult(const UsersPage& result)
    {
        m_getUsersResult = result;
    }

    void setGetUserResult(std::optional<dto::User> user)
    {
        m_getUserResult = std::move(user);
    }

    void setCreateUserResult(std::optional<dto::User> user)
    {
        m_createUserResult = std::move(user);
    }

    void setUpdateUserResult(std::optional<dto::User> user)
    {
        m_updateUserResult = std::move(user);
    }

    void setDeleteUserResult(bool result)
    {
        m_deleteUserResult = result;
    }

    // Реализация интерфейса
    UsersPage users(int page, int pageSize) override
    {
        m_lastGetUsersPage = page;
        m_lastGetUsersPageSize = pageSize;
        m_getUsersCallCount++;
        return m_getUsersResult;
    }

    std::optional<dto::User> user(int64_t id) override
    {
        m_lastGetUserId = id;
        m_getUserCallCount++;
        return m_getUserResult;
    }

    std::optional<dto::User> createUser(const dto::User& user, const std::string& password) override
    {
        m_lastCreatedUser = user;
        m_lastCreatedPassword = password;
        m_createUserCallCount++;
        return m_createUserResult;
    }

    std::optional<dto::User> updateUser(const dto::User& user) override
    {
        m_lastUpdatedUser = user;
        m_updateUserCallCount++;
        return m_updateUserResult;
    }

    bool deleteUser(int64_t id) override
    {
        m_lastDeletedUserId = id;
        m_deleteUserCallCount++;
        return m_deleteUserResult;
    }

    // Методы для проверки вызовов
    int getUsersCallCount() const { return m_getUsersCallCount; }
    int getUserCallCount() const { return m_getUserCallCount; }
    int createUserCallCount() const { return m_createUserCallCount; }
    int updateUserCallCount() const { return m_updateUserCallCount; }
    int deleteUserCallCount() const { return m_deleteUserCallCount; }

    int getLastGetUsersPage() const { return m_lastGetUsersPage; }
    int getLastGetUsersPageSize() const { return m_lastGetUsersPageSize; }
    int64_t getLastGetUserId() const { return m_lastGetUserId; }
    const dto::User& getLastCreatedUser() const { return m_lastCreatedUser; }
    const std::string& getLastCreatedPassword() const { return m_lastCreatedPassword; }
    const dto::User& getLastUpdatedUser() const { return m_lastUpdatedUser; }
    int64_t getLastDeletedUserId() const { return m_lastDeletedUserId; }

    void reset()
    {
        m_getUsersCallCount = 0;
        m_getUserCallCount = 0;
        m_createUserCallCount = 0;
        m_updateUserCallCount = 0;
        m_deleteUserCallCount = 0;

        m_lastGetUsersPage = 0;
        m_lastGetUsersPageSize = 0;
        m_lastGetUserId = 0;
        m_lastCreatedUser = dto::User {};
        m_lastCreatedPassword.clear();
        m_lastUpdatedUser = dto::User {};
        m_lastDeletedUserId = 0;
    }

private:
    UsersPage m_getUsersResult;
    std::optional<dto::User> m_getUserResult;
    std::optional<dto::User> m_createUserResult;
    std::optional<dto::User> m_updateUserResult;
    bool m_deleteUserResult = false;

    int m_getUsersCallCount = 0;
    int m_getUserCallCount = 0;
    int m_createUserCallCount = 0;
    int m_updateUserCallCount = 0;
    int m_deleteUserCallCount = 0;

    int m_lastGetUsersPage = 0;
    int m_lastGetUsersPageSize = 0;
    int64_t m_lastGetUserId = 0;
    dto::User m_lastCreatedUser;
    std::string m_lastCreatedPassword;
    dto::User m_lastUpdatedUser;
    int64_t m_lastDeletedUserId = 0;
};

} // namespace tests
} // namespace server
