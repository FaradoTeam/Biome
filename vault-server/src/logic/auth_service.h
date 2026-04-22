// services/auth_service.h
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "common/dto/user.h"

#include "api/middleware/auth_middleware.h"

#include "repo/user_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Результат аутентификации.
 */
struct AuthResult
{
    bool success = false;
    std::string accessToken;
    std::string tokenType = "Bearer";
    int expiresIn = 3600;
    std::string errorMessage;
    int errorCode = 0;
};

/**
 * @brief Результат смены пароля.
 */
struct ChangePasswordResult
{
    bool success = false;
    std::string errorMessage;
    int errorCode = 0;
};

/**
 * @brief Сервис аутентификации и управления пользователями.
 */
class AuthService
{
public:
    AuthService(
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<AuthMiddleware> authMiddleware
    );

    ~AuthService() = default;

    AuthResult login(const std::string& login, const std::string& password);
    bool logout(const std::string& token);

    ChangePasswordResult changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    );

    bool verifyPassword(int64_t userId, const std::string& password);
    std::string getPasswordHash(int64_t userId);

private:
    std::string hashPassword(const std::string& password);
    bool checkPassword(const std::string& password, const std::string& hash);
    bool validatePasswordStrength(const std::string& password);

private:
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<AuthMiddleware> m_authMiddleware;

    static constexpr int DEFAULT_TOKEN_EXPIRY = 3600; // 1 час
    static constexpr size_t MIN_PASSWORD_LENGTH = 8;
};

} // namespace services
} // namespace server
