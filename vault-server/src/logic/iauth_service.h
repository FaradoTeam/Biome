#pragma once

#include <string>

namespace server
{
namespace services
{

struct AuthResult
{
    bool success = false;
    int expiresIn = 3600;
    int errorCode = 0;
    std::string accessToken;
    std::string tokenType = "Bearer";
    std::string errorMessage;
};

struct ChangePasswordResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

class IAuthService
{
public:
    virtual ~IAuthService() = default;

public:
    virtual AuthResult login(
        const std::string& login,
        const std::string& password
    ) = 0;

    virtual bool logout(
        const std::string& token
    ) = 0;

    virtual ChangePasswordResult changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    ) = 0;

    virtual bool verifyPassword(
        int64_t userId,
        const std::string& password
    ) = 0;
};

} // namespace services
} // namespace server
