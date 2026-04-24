#pragma once

#include <functional>
#include <optional>
#include <string>

#include "common/dto/user.h"

#include "repo/user_repository.h"

namespace server
{
namespace tests
{

class MockUserRepository : public repositories::IUserRepository
{
public:
    void setFindByLoginResult(std::optional<dto::User> user)
    {
        m_findByLoginResult = std::move(user);
    }

    void setFindByIdResult(std::optional<dto::User> user)
    {
        m_findByIdResult = std::move(user);
    }

    void setPasswordHashResult(const std::string& hash)
    {
        m_passwordHashResult = hash;
    }

    void setUpdatePasswordResult(bool result)
    {
        m_updatePasswordResult = result;
    }

    void setUpdateNeedChangePasswordResult(bool result)
    {
        m_updateNeedChangePasswordResult = result;
    }

    void setCreateResult(int64_t id)
    {
        m_createResult = id;
    }

    void setExistsByLoginResult(bool result)
    {
        m_existsByLoginResult = result;
    }

    // Реализация интерфейса
    std::optional<dto::User> findByLogin(const std::string& login) override
    {
        m_lastFindByLogin = login;
        m_findByLoginCallCount++;
        return m_findByLoginResult;
    }

    std::optional<dto::User> findById(int64_t id) override
    {
        m_lastFindById = id;
        m_findByIdCallCount++;
        return m_findByIdResult;
    }

    std::string passwordHash(int64_t userId) override
    {
        m_lastPasswordHashUserId = userId;
        m_passwordHashCallCount++;
        return m_passwordHashResult;
    }

    bool updatePassword(int64_t userId, const std::string& passwordHash) override
    {
        m_lastUpdatePasswordUserId = userId;
        m_lastUpdatePasswordHash = passwordHash;
        m_updatePasswordCallCount++;
        return m_updatePasswordResult;
    }

    bool updateNeedChangePassword(int64_t userId, bool needChange) override
    {
        m_lastUpdateNeedChangeUserId = userId;
        m_lastNeedChange = needChange;
        m_updateNeedChangeCallCount++;
        return m_updateNeedChangePasswordResult;
    }

    int64_t create(const dto::User& user, const std::string& passwordHash) override
    {
        m_lastCreateUser = user;
        m_lastCreatePasswordHash = passwordHash;
        m_createCallCount++;
        return m_createResult;
    }

    bool existsByLogin(const std::string& login) override
    {
        m_lastExistsByLogin = login;
        m_existsByLoginCallCount++;
        return m_existsByLoginResult;
    }

    // Методы для проверки
    int getFindByLoginCallCount() const { return m_findByLoginCallCount; }
    int getFindByIdCallCount() const { return m_findByIdCallCount; }
    int getPasswordHashCallCount() const { return m_passwordHashCallCount; }
    int getUpdatePasswordCallCount() const { return m_updatePasswordCallCount; }

    const std::string& getLastFindByLogin() const { return m_lastFindByLogin; }
    int64_t getLastFindById() const { return m_lastFindById; }

    void reset()
    {
        m_findByLoginCallCount = 0;
        m_findByIdCallCount = 0;
        m_passwordHashCallCount = 0;
        m_updatePasswordCallCount = 0;
        m_updateNeedChangeCallCount = 0;
        m_createCallCount = 0;
        m_existsByLoginCallCount = 0;
    }

private:
    std::optional<dto::User> m_findByLoginResult;
    std::optional<dto::User> m_findByIdResult;
    std::string m_passwordHashResult;
    bool m_updatePasswordResult = false;
    bool m_updateNeedChangePasswordResult = false;
    int64_t m_createResult = 0;
    bool m_existsByLoginResult = false;

    int m_findByLoginCallCount = 0;
    int m_findByIdCallCount = 0;
    int m_passwordHashCallCount = 0;
    int m_updatePasswordCallCount = 0;
    int m_updateNeedChangeCallCount = 0;
    int m_createCallCount = 0;
    int m_existsByLoginCallCount = 0;

    std::string m_lastFindByLogin;
    int64_t m_lastFindById = 0;
    int64_t m_lastPasswordHashUserId = 0;
    int64_t m_lastUpdatePasswordUserId = 0;
    std::string m_lastUpdatePasswordHash;
    int64_t m_lastUpdateNeedChangeUserId = 0;
    bool m_lastNeedChange = false;
    dto::User m_lastCreateUser;
    std::string m_lastCreatePasswordHash;
    std::string m_lastExistsByLogin;
};

} // namespace tests
} // namespace server
