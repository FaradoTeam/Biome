#include <cctype>
#include <iomanip>
#include <sstream>

#include <openssl/evp.h>

#include "common/log/log.h"

#include "common/helpers/crypto_helper.hpp"

#include "auth_service.h"

namespace
{
constexpr int DEFAULT_TOKEN_EXPIRY = 3600; // 1 час
constexpr size_t MIN_PASSWORD_LENGTH = 8;
} // namespace

namespace server
{
namespace services
{

AuthService::AuthService(
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<IAuthMiddleware> authMiddleware
)
    : m_userRepo(std::move(userRepo))
    , m_authMiddleware(std::move(authMiddleware))
{
    if (!m_userRepo)
    {
        throw std::runtime_error("UserRepository cannot be null");
    }
    if (!m_authMiddleware)
    {
        throw std::runtime_error("AuthMiddleware cannot be null");
    }
}

AuthResult AuthService::login(const std::string& login, const std::string& password)
{
    AuthResult result;

    // 1. Проверяем входные данные
    if (login.empty() || password.empty())
    {
        result.errorMessage = "Login and password are required";
        result.errorCode = 400;
        LOG_WARN << "Попытка входа с пустым логином или паролем";
        return result;
    }

    // 2. Ищем пользователя по логину
    auto userOpt = m_userRepo->findByLogin(login);
    if (!userOpt.has_value())
    {
        result.errorMessage = "Invalid credentials";
        result.errorCode = 401;
        LOG_WARN << "Неудачная попытка входа: пользователь '" << login << "' не найден";
        return result;
    }

    const auto& user = userOpt.value();

    // 3. Проверяем, не заблокирован ли пользователь
    if (user.isBlocked.value_or(false))
    {
        result.errorMessage = "Account is blocked";
        result.errorCode = 403;
        LOG_WARN << "Попытка входа заблокированного пользователя: " << login;
        return result;
    }

    // 4. Проверяем пароль
    if (!user.id.has_value())
    {
        result.errorMessage = "Invalid user data";
        result.errorCode = 500;
        LOG_ERROR << "Пользователь без ID: " << login;
        return result;
    }

    if (!verifyPassword(user.id.value(), password))
    {
        result.errorMessage = "Invalid credentials";
        result.errorCode = 401;
        LOG_WARN << "Неверный пароль для пользователя: " << login;
        return result;
    }

    // 5. Генерируем JWT-токен
    const std::string userIdStr = std::to_string(user.id.value());
    result.accessToken = m_authMiddleware->generateToken(userIdStr, DEFAULT_TOKEN_EXPIRY);
    result.success = true;
    result.tokenType = "Bearer";
    result.expiresIn = DEFAULT_TOKEN_EXPIRY;

    LOG_INFO
        << "Пользователь успешно вошел в систему: " << login
        << " (id=" << user.id.value() << ")";

    return result;
}

bool AuthService::logout(const std::string& token)
{
    if (token.empty())
    {
        LOG_WARN << "Попытка выхода с пустым токеном";
        return false;
    }

    m_authMiddleware->invalidateToken(token);
    LOG_INFO << "Пользователь вышел из системы, токен аннулирован";
    return true;
}

ChangePasswordResult AuthService::changePassword(
    int64_t userId,
    const std::string& oldPassword,
    const std::string& newPassword
)
{
    ChangePasswordResult result;

    // 1. Проверяем входные данные
    if (oldPassword.empty() || newPassword.empty())
    {
        result.errorMessage = "Old password and new password are required";
        result.errorCode = 400;
        return result;
    }

    // 2. Проверяем существование пользователя
    auto userOpt = m_userRepo->findById(userId);
    if (!userOpt.has_value())
    {
        result.errorMessage = "User not found";
        result.errorCode = 404;
        return result;
    }

    // 3. Проверяем старый пароль
    if (!verifyPassword(userId, oldPassword))
    {
        result.errorMessage = "Invalid old password";
        result.errorCode = 401;
        LOG_WARN
            << "Неверный старый пароль при смене пароля для пользователя id="
            << userId;
        return result;
    }

    // 4. Проверяем, что новый пароль отличается от старого
    if (oldPassword == newPassword)
    {
        result.errorMessage = "New password must be different from old password";
        result.errorCode = 400;
        return result;
    }

    // 5. Валидируем сложность нового пароля
    if (!validatePasswordStrength(newPassword))
    {
        result.errorMessage = "Password must be at least 8 characters long and "
                              "contain uppercase, lowercase and digit";
        result.errorCode = 400;
        return result;
    }

    // 6. Хешируем и сохраняем новый пароль
    const std::string newHash = crypto::sha256(newPassword);
    if (!m_userRepo->updatePassword(userId, newHash))
    {
        result.errorMessage = "Failed to update password";
        result.errorCode = 500;
        LOG_ERROR
            << "Не удалось обновить пароль для пользователя id="
            << userId;
        return result;
    }

    // 7. Сбрасываем флаг необходимости смены пароля (если он был установлен)
    m_userRepo->updateNeedChangePassword(userId, false);

    result.success = true;
    LOG_INFO << "Пароль успешно изменен для пользователя id=" << userId;

    return result;
}

bool AuthService::verifyPassword(int64_t userId, const std::string& password)
{
    std::string storedHash = passwordHash(userId);
    if (storedHash.empty())
    {
        LOG_WARN << "Не найден хеш пароля для пользователя id=" << userId;
        return false;
    }
    return checkPassword(password, storedHash);
}

std::string AuthService::passwordHash(int64_t userId)
{
    return m_userRepo->passwordHash(userId);
}

bool AuthService::checkPassword(
    const std::string& password,
    const std::string& hash
)
{
    return crypto::sha256(password) == hash;
}

bool AuthService::validatePasswordStrength(const std::string& password)
{
    if (password.length() < MIN_PASSWORD_LENGTH)
    {
        return false;
    }

    bool hasUpper = false;
    bool hasLower = false;
    bool hasDigit = false;

    for (char character : password)
    {
        if (std::isupper(static_cast<unsigned char>(character)))
            hasUpper = true;
        if (std::islower(static_cast<unsigned char>(character)))
            hasLower = true;
        if (std::isdigit(static_cast<unsigned char>(character)))
            hasDigit = true;
    }

    // Требуем хотя бы одну заглавную, одну строчную и одну цифру
    return hasUpper && hasLower && hasDigit;
}

} // namespace services
} // namespace server
