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
        m_getUsersCallback = nullptr;
    }

    void setGetUserResult(std::optional<dto::User> user)
    {
        m_getUserResult = std::move(user);
        m_getUserCallback = nullptr;
    }

    void setCreateUserResult(std::optional<dto::User> user)
    {
        m_createUserResult = std::move(user);
        m_createUserCallback = nullptr;
    }

    void setUpdateUserResult(std::optional<dto::User> user)
    {
        m_updateUserResult = std::move(user);
        m_updateUserCallback = nullptr;
    }

    void setDeleteUserResult(bool result)
    {
        m_deleteUserResult = result;
        m_deleteUserCallback = nullptr;
    }

    // Callback-и для кастомной логики
    void setGetUsersCallback(
        std::function<UsersPage(int, int, int64_t)> callback
    )
    {
        m_getUsersCallback = std::move(callback);
    }

    void setGetUserCallback(
        std::function<std::optional<dto::User>(int64_t, int64_t)> callback
    )
    {
        m_getUserCallback = std::move(callback);
    }

    void setCreateUserCallback(
        std::function<std::optional<dto::User>(const dto::User&, const std::string&, int64_t)> callback
    )
    {
        m_createUserCallback = std::move(callback);
    }

    void setUpdateUserCallback(
        std::function<std::optional<dto::User>(const dto::User&, int64_t)> callback
    )
    {
        m_updateUserCallback = std::move(callback);
    }

    void setDeleteUserCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteUserCallback = std::move(callback);
    }

    // Реализация интерфейса IUserService
    services::UsersPage users(
        int page,
        int pageSize,
        int64_t userId
    ) override
    {
        m_lastGetUsersPage = page;
        m_lastGetUsersPageSize = pageSize;
        m_lastGetUsersUserId = userId;
        m_getUsersCallCount++;

        if (m_getUsersCallback)
        {
            return m_getUsersCallback(page, pageSize, userId);
        }

        // Любой авторизованный пользователь может получить список пользователей
        // Возвращаем результат независимо от userId
        return m_getUsersResult;
    }

    std::optional<dto::User> user(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastGetUserId = id;
        m_lastGetUserRequestUserId = userId;
        m_getUserCallCount++;

        if (m_getUserCallback)
        {
            return m_getUserCallback(id, userId);
        }

        // Любой авторизованный пользователь может получить информацию о пользователе
        // Возвращаем результат независимо от userId
        if (m_getUserResult.has_value() && m_getUserResult->id.has_value())
        {
            if (*m_getUserResult->id == id)
            {
                return m_getUserResult;
            }
        }
        return std::nullopt;
    }

    std::optional<dto::User> createUser(
        const dto::User& user,
        const std::string& password,
        int64_t userId
    ) override
    {
        m_lastCreatedUser = user;
        m_lastCreatedPassword = password;
        m_lastCreateUserUserId = userId;
        m_createUserCallCount++;

        if (m_createUserCallback)
        {
            return m_createUserCallback(user, password, userId);
        }

        // Только супер-админ (userId=1) может создавать пользователей
        if (userId != 1)
        {
            return std::nullopt;
        }

        if (m_createUserResult.has_value())
        {
            if (!m_createUserResult->id.has_value())
            {
                m_createUserResult->id = m_nextId++;
            }
        }

        return m_createUserResult;
    }

    std::optional<dto::User> updateUser(
        const dto::User& user,
        int64_t userId
    ) override
    {
        m_lastUpdatedUser = user;
        m_lastUpdateUserUserId = userId;
        m_updateUserCallCount++;

        if (m_updateUserCallback)
        {
            return m_updateUserCallback(user, userId);
        }

        // Только супер-админ (userId=1) может обновлять пользователей
        if (userId != 1)
        {
            return std::nullopt;
        }

        return m_updateUserResult;
    }

    bool deleteUser(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedUserId = id;
        m_lastDeleteUserUserId = userId;
        m_deleteUserCallCount++;

        if (m_deleteUserCallback)
        {
            return m_deleteUserCallback(id, userId);
        }

        // Только супер-админ (userId=1) может удалять пользователей
        if (userId != 1)
        {
            return false;
        }

        return m_deleteUserResult;
    }

    // Методы для проверки вызовов
    int getGetUsersCallCount() const { return m_getUsersCallCount; }
    int getGetUserCallCount() const { return m_getUserCallCount; }
    int getCreateUserCallCount() const { return m_createUserCallCount; }
    int getUpdateUserCallCount() const { return m_updateUserCallCount; }
    int getDeleteUserCallCount() const { return m_deleteUserCallCount; }

    int getLastGetUsersPage() const { return m_lastGetUsersPage; }
    int getLastGetUsersPageSize() const { return m_lastGetUsersPageSize; }
    int64_t getLastGetUsersUserId() const { return m_lastGetUsersUserId; }
    int64_t getLastGetUserId() const { return m_lastGetUserId; }
    int64_t getLastGetUserRequestUserId() const { return m_lastGetUserRequestUserId; }
    const dto::User& getLastCreatedUser() const { return m_lastCreatedUser; }
    const std::string& getLastCreatedPassword() const { return m_lastCreatedPassword; }
    int64_t getLastCreateUserUserId() const { return m_lastCreateUserUserId; }
    const dto::User& getLastUpdatedUser() const { return m_lastUpdatedUser; }
    int64_t getLastUpdateUserUserId() const { return m_lastUpdateUserUserId; }
    int64_t getLastDeletedUserId() const { return m_lastDeletedUserId; }
    int64_t getLastDeleteUserUserId() const { return m_lastDeleteUserUserId; }

    void reset()
    {
        m_getUsersCallCount = 0;
        m_getUserCallCount = 0;
        m_createUserCallCount = 0;
        m_updateUserCallCount = 0;
        m_deleteUserCallCount = 0;

        m_lastGetUsersPage = 0;
        m_lastGetUsersPageSize = 0;
        m_lastGetUsersUserId = 0;
        m_lastGetUserId = 0;
        m_lastGetUserRequestUserId = 0;
        m_lastCreatedUser = dto::User {};
        m_lastCreatedPassword.clear();
        m_lastCreateUserUserId = 0;
        m_lastUpdatedUser = dto::User {};
        m_lastUpdateUserUserId = 0;
        m_lastDeletedUserId = 0;
        m_lastDeleteUserUserId = 0;

        m_getUsersCallback = nullptr;
        m_getUserCallback = nullptr;
        m_createUserCallback = nullptr;
        m_updateUserCallback = nullptr;
        m_deleteUserCallback = nullptr;

        m_getUsersResult = services::UsersPage {};
        m_getUserResult = std::nullopt;
        m_createUserResult = std::nullopt;
        m_updateUserResult = std::nullopt;
        m_deleteUserResult = false;
        m_nextId = 100;
    }

private:
    services::UsersPage m_getUsersResult;
    std::optional<dto::User> m_getUserResult;
    std::optional<dto::User> m_createUserResult;
    std::optional<dto::User> m_updateUserResult;
    bool m_deleteUserResult = false;

    // Callback-и
    std::function<services::UsersPage(int, int, int64_t)> m_getUsersCallback;
    std::function<std::optional<dto::User>(int64_t, int64_t)> m_getUserCallback;
    std::function<std::optional<dto::User>(const dto::User&, const std::string&, int64_t)> m_createUserCallback;
    std::function<std::optional<dto::User>(const dto::User&, int64_t)> m_updateUserCallback;
    std::function<bool(int64_t, int64_t)> m_deleteUserCallback;

    // Счётчики вызовов
    int m_getUsersCallCount = 0;
    int m_getUserCallCount = 0;
    int m_createUserCallCount = 0;
    int m_updateUserCallCount = 0;
    int m_deleteUserCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetUsersPage = 0;
    int m_lastGetUsersPageSize = 0;
    int64_t m_lastGetUsersUserId = 0;
    int64_t m_lastGetUserId = 0;
    int64_t m_lastGetUserRequestUserId = 0;
    dto::User m_lastCreatedUser;
    std::string m_lastCreatedPassword;
    int64_t m_lastCreateUserUserId = 0;
    dto::User m_lastUpdatedUser;
    int64_t m_lastUpdateUserUserId = 0;
    int64_t m_lastDeletedUserId = 0;
    int64_t m_lastDeleteUserUserId = 0;
    int64_t m_nextId = 100;
};

} // namespace tests
} // namespace server
