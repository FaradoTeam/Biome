#pragma once

#include <string>

namespace server
{

class IAuthMiddleware
{
public:
    virtual ~IAuthMiddleware() = default;

public:
    virtual bool validateRequest(
        const std::string& authHeader,
        std::string& userId
    ) = 0;

    virtual std::string generateToken(
        const std::string& userId,
        int expiresInSeconds = 3600
    ) = 0;

    virtual void invalidateToken(const std::string& token) = 0;

    virtual bool isTokenInvalidated(const std::string& token) = 0;
};

} // namespace server
