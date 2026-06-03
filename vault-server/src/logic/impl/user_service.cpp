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
        throw std::runtime_error(
            "UserService: репозиторий пользователей не инициализирован"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error(
            "UserService: сервис авторизации не инициализирован"
        );
    }
}

UsersPage UserService::users(int page, int pageSize, int64_t /*userId*/)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [users, total] = m_userRepo->findAll(page, pageSize);
    return { users, total };
}

std::optional<dto::User> UserService::user(int64_t id, int64_t userId)
{
    return m_userRepo->findById(id);
}

std::optional<dto::User> UserService::createUser(
    const dto::User& user,
    const std::string& password,
    int64_t userId
)
{
    // Только супер-админ может создавать пользователей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createUser: пользователь " << userId << " не имеет прав на создание пользователей";
        return std::nullopt;
    }

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
        LOG_ERROR << "createUser: не удалось создать пользователя";
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь создан: id=" << newId
        << ", логин=" << *user.login
        << ", пользователь=" << userId;

    return m_userRepo->findById(newId);
}

std::optional<dto::User> UserService::updateUser(
    const dto::User& user,
    int64_t userId
)
{
    // Только супер-админ может обновлять пользователей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateUser: пользователь " << userId << " не имеет прав на обновление пользователей";
        return std::nullopt;
    }

    if (!user.id.has_value())
    {
        LOG_WARN << "updateUser: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_userRepo->findById(*user.id);
    if (!existing)
    {
        LOG_WARN << "updateUser: пользователь не найден, id=" << *user.id;
        return std::nullopt;
    }

    // Сохраняем старые значения для инвалидации
    const bool wasSuperAdmin = existing->isSuperAdmin.value_or(false);
    const bool wasBlocked = existing->isBlocked.value_or(false);
    const bool wasHidden = existing->isHidden.value_or(false);

    if (!m_userRepo->update(user))
    {
        LOG_ERROR << "updateUser: не удалось обновить пользователя id=" << *user.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Пользователь обновлен: id=" << *user.id
        << ", пользователь=" << userId;

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

bool UserService::deleteUser(int64_t id, int64_t userId)
{
    // Только супер-админ может удалять пользователей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteUser: пользователь " << userId << " не имеет прав на удаление пользователей";
        return false;
    }

    auto existing = m_userRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteUser: пользователь не найден, id=" << id;
        return false;
    }

    // Нельзя удалить супер-админа
    if (existing->isSuperAdmin.value_or(false))
    {
        LOG_WARN << "deleteUser: нельзя удалить супер-администратора id=" << id;
        return false;
    }

    if (!m_userRepo->remove(id))
    {
        LOG_ERROR << "deleteUser: не удалось удалить пользователя id=" << id;
        return false;
    }

    LOG_INFO
        << "Пользователь удален: id=" << id
        << ", пользователь=" << userId;

    // Инвалидируем кэш удалённого пользователя
    m_authzService->invalidateCache(id);

    return true;
}

} // namespace server::services
