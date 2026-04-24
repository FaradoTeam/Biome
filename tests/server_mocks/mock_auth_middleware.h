#pragma once

#include <functional>
#include <string>

#include "api/middleware/iauth_middleware.h"

namespace server
{
namespace tests
{

class MockAuthMiddleware : public IAuthMiddleware
{
public:
    void setValidateRequestResult(bool result, const std::string& userId = "test_user")
    {
        m_validateResult = result;
        m_validateUserId = userId;
        m_validateCallback = [result, userId](const std::string&, std::string& outUserId)
        {
            outUserId = userId;
            return result;
        };
    }

    void setValidateRequestCallback(
        std::function<bool(const std::string&, std::string&)> callback
    )
    {
        m_validateCallback = std::move(callback);
    }

    void setGenerateTokenResult(const std::string& token)
    {
        m_generateTokenResult = token;
    }

    void setInvalidateTokenCallback(std::function<void(const std::string&)> callback)
    {
        m_invalidateCallback = std::move(callback);
    }

    void setIsTokenInvalidatedResult(bool result)
    {
        m_isInvalidatedResult = result;
    }

    // Реализация интерфейса
    bool validateRequest(const std::string& authHeader, std::string& userId) override
    {
        m_lastAuthHeader = authHeader;
        m_validateCallCount++;

        if (m_validateCallback)
        {
            return m_validateCallback(authHeader, userId);
        }

        userId = m_validateUserId;
        return m_validateResult;
    }

    std::string generateToken(const std::string& userId, int expiresInSeconds) override
    {
        m_lastGenerateUserId = userId;
        m_lastExpiresIn = expiresInSeconds;
        m_generateCallCount++;
        return m_generateTokenResult;
    }

    void invalidateToken(const std::string& token) override
    {
        m_lastInvalidateToken = token;
        m_invalidateCallCount++;

        if (m_invalidateCallback)
        {
            m_invalidateCallback(token);
        }
    }

    bool isTokenInvalidated(const std::string& token) override
    {
        m_lastCheckToken = token;
        m_isInvalidatedCallCount++;
        return m_isInvalidatedResult;
    }

    // Методы для проверки
    int getValidateCallCount() const { return m_validateCallCount; }
    int getGenerateCallCount() const { return m_generateCallCount; }
    int getInvalidateCallCount() const { return m_invalidateCallCount; }

    const std::string& getLastAuthHeader() const { return m_lastAuthHeader; }
    const std::string& getLastGenerateUserId() const { return m_lastGenerateUserId; }
    const std::string& getLastInvalidateToken() const { return m_lastInvalidateToken; }

    void reset()
    {
        m_validateCallCount = 0;
        m_generateCallCount = 0;
        m_invalidateCallCount = 0;
        m_isInvalidatedCallCount = 0;
        m_validateCallback = nullptr;
        m_invalidateCallback = nullptr;
    }

private:
    bool m_validateResult = false;
    std::string m_validateUserId;
    std::string m_generateTokenResult;
    bool m_isInvalidatedResult = false;

    std::function<bool(const std::string&, std::string&)> m_validateCallback;
    std::function<void(const std::string&)> m_invalidateCallback;

    int m_validateCallCount = 0;
    int m_generateCallCount = 0;
    int m_invalidateCallCount = 0;
    int m_isInvalidatedCallCount = 0;

    std::string m_lastAuthHeader;
    std::string m_lastGenerateUserId;
    int m_lastExpiresIn = 0;
    std::string m_lastInvalidateToken;
    std::string m_lastCheckToken;
};

} // namespace tests
} // namespace server
