#pragma once

#include <functional>
#include <string>

#include "logic/iauth_service.h"

namespace server
{
namespace tests
{

class MockAuthService : public services::IAuthService
{
public:
    using AuthResult = services::AuthResult;
    using ChangePasswordResult = services::ChangePasswordResult;

    void setLoginResult(const AuthResult& result)
    {
        m_loginResult = result;
        m_loginCallback = [result](const std::string&, const std::string&)
        {
            return result;
        };
    }

    void setLoginCallback(std::function<AuthResult(const std::string&, const std::string&)> callback)
    {
        m_loginCallback = std::move(callback);
    }

    void setLogoutResult(bool result)
    {
        m_logoutResult = result;
        m_logoutCallback = [result](const std::string&)
        {
            return result;
        };
    }

    void setChangePasswordResult(const ChangePasswordResult& result)
    {
        m_changePasswordResult = result;
    }

    void setVerifyPasswordResult(bool result)
    {
        m_verifyPasswordResult = result;
    }

    // Реализация интерфейса
    AuthResult login(const std::string& login, const std::string& password) override
    {
        m_lastLogin = login;
        m_lastPassword = password;
        m_loginCallCount++;

        if (m_loginCallback)
        {
            return m_loginCallback(login, password);
        }
        return m_loginResult;
    }

    bool logout(const std::string& token) override
    {
        m_lastToken = token;
        m_logoutCallCount++;

        if (m_logoutCallback)
        {
            return m_logoutCallback(token);
        }
        return m_logoutResult;
    }

    ChangePasswordResult changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    ) override
    {
        m_lastUserId = userId;
        m_lastOldPassword = oldPassword;
        m_lastNewPassword = newPassword;
        m_changePasswordCallCount++;

        return m_changePasswordResult;
    }

    bool verifyPassword(int64_t userId, const std::string& password) override
    {
        m_lastVerifyUserId = userId;
        m_lastVerifyPassword = password;
        m_verifyPasswordCallCount++;

        return m_verifyPasswordResult;
    }

    // Методы для проверки вызовов
    int getLoginCallCount() const { return m_loginCallCount; }
    int getLogoutCallCount() const { return m_logoutCallCount; }
    int getChangePasswordCallCount() const { return m_changePasswordCallCount; }
    int getVerifyPasswordCallCount() const { return m_verifyPasswordCallCount; }

    const std::string& getLastLogin() const { return m_lastLogin; }
    const std::string& getLastPassword() const { return m_lastPassword; }
    const std::string& getLastToken() const { return m_lastToken; }
    int64_t getLastUserId() const { return m_lastUserId; }

    void reset()
    {
        m_loginCallCount = 0;
        m_logoutCallCount = 0;
        m_changePasswordCallCount = 0;
        m_verifyPasswordCallCount = 0;
        m_lastLogin.clear();
        m_lastPassword.clear();
        m_lastToken.clear();
        m_loginCallback = nullptr;
        m_logoutCallback = nullptr;
    }

private:
    AuthResult m_loginResult;
    ChangePasswordResult m_changePasswordResult;
    bool m_logoutResult = false;
    bool m_verifyPasswordResult = false;

    std::function<AuthResult(const std::string&, const std::string&)> m_loginCallback;
    std::function<bool(const std::string&)> m_logoutCallback;

    int m_loginCallCount = 0;
    int m_logoutCallCount = 0;
    int m_changePasswordCallCount = 0;
    int m_verifyPasswordCallCount = 0;

    std::string m_lastLogin;
    std::string m_lastPassword;
    std::string m_lastToken;
    int64_t m_lastUserId = 0;
    std::string m_lastOldPassword;
    std::string m_lastNewPassword;
    int64_t m_lastVerifyUserId = 0;
    std::string m_lastVerifyPassword;
};

} // namespace tests
} // namespace server
