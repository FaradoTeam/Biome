// services/auth_service.h
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "common/dto/user.h"

#include "api/middleware/iauth_middleware.h"

#include "logic/iauth_service.h"

#include "repo/user_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Сервис аутентификации и управления пользователями.
 */
class AuthService final : public IAuthService
{
public:
    AuthService(
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<IAuthMiddleware> authMiddleware
    );

    AuthResult login(
        const std::string& login,
        const std::string& password
    ) override;

    bool logout(
        const std::string& token
    ) override;

    ChangePasswordResult changePassword(
        int64_t userId,
        const std::string& oldPassword,
        const std::string& newPassword
    ) override;

    bool verifyPassword(
        int64_t userId,
        const std::string& password
    ) override;

private:
    std::string passwordHash(int64_t userId);
    bool checkPassword(const std::string& password, const std::string& hash);
    bool validatePasswordStrength(const std::string& password);

private:
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<IAuthMiddleware> m_authMiddleware;
};

} // namespace services
} // namespace server
