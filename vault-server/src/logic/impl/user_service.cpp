#include <iomanip>
#include <sstream>

#include <openssl/evp.h>

#include "common/helpers/crypto_helper.hpp"
#include "common/log/log.h"

#include "user_service.h"

namespace server::services
{

UserService::UserService(
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_userRepo(std::move(userRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_userRepo)
    {
        throw std::runtime_error("UserService: userRepository is null");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserService: authorizationService is null");
    }
}

UsersPage UserService::users(int page, int pageSize)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [users, total] = m_userRepo->findAll(page, pageSize);
    return { users, total };
}

std::optional<dto::User> UserService::user(int64_t id)
{
    return m_userRepo->findById(id);
}

std::optional<dto::User> UserService::createUser(
    const dto::User& user,
    const std::string& password
)
{
    if (!user.login.has_value()
        || user.login->empty()
        || !user.email.has_value()
        || user.email->empty()
        || password.empty())
    {
        LOG_WARN << "createUser: не все обязательные поля заполнены";
        return std::nullopt;
    }

    // Проверяем уникальность логина
    if (m_userRepo->existsByLogin(*user.login))
    {
        LOG_WARN
            << "createUser: пользователь с логином '" << *user.login
            << "' уже существует";
        return std::nullopt;
    }

    const std::string hashedPassword = crypto::sha256(password);
    const int64_t newId = m_userRepo->create(user, hashedPassword);

    if (newId <= 0)
    {
        LOG_ERROR << "createUser: failed to create user";
        return std::nullopt;
    }

    LOG_INFO << "User created: id=" << newId << ", login=" << *user.login;

    return m_userRepo->findById(newId);
}

std::optional<dto::User> UserService::updateUser(const dto::User& user)
{
    if (!user.id.has_value())
    {
        LOG_WARN << "updateUser: missing id";
        return std::nullopt;
    }

    auto existing = m_userRepo->findById(*user.id);
    if (!existing)
    {
        LOG_WARN << "updateUser: user not found, id=" << *user.id;
        return std::nullopt;
    }

    // Сохраняем старые значения для инвалидации
    const bool wasSuperAdmin = existing->isSuperAdmin.value_or(false);
    const bool wasBlocked = existing->isBlocked.value_or(false);
    const bool wasHidden = existing->isHidden.value_or(false);

    if (!m_userRepo->update(user))
    {
        LOG_ERROR << "updateUser: failed to update user id=" << *user.id;
        return std::nullopt;
    }

    LOG_INFO << "User updated: id=" << *user.id;

    // Проверяем, изменились ли права доступа
    const bool isSuperAdminChanged = user.isSuperAdmin.has_value() && *user.isSuperAdmin != wasSuperAdmin;
    const bool isBlockedChanged = user.isBlocked.has_value() && *user.isBlocked != wasBlocked;
    const bool isHiddenChanged = user.isHidden.has_value() && *user.isHidden != wasHidden;

    // Если изменились права, инвалидируем кэш
    if (isSuperAdminChanged || isBlockedChanged || isHiddenChanged)
    {
        m_authzService->invalidateCache(*user.id);
        LOG_DEBUG
            << "Инвалидирован кэш для пользователя " << *user.id
            << " из-за изменения прав";
    }

    return m_userRepo->findById(*user.id);
}

bool UserService::deleteUser(int64_t id)
{
    auto existing = m_userRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteUser: user not found, id=" << id;
        return false;
    }

    if (!m_userRepo->remove(id))
    {
        LOG_ERROR << "deleteUser: failed to delete user id=" << id;
        return false;
    }

    LOG_INFO << "User deleted: id=" << id;

    // Инвалидируем кэш удалённого пользователя
    m_authzService->invalidateCache(id);

    return true;
}

} // namespace server::services
