#include <iomanip>
#include <sstream>

#include <openssl/evp.h>

#include "common/log/log.h"

#include "common/helpers/crypto_helper.hpp"

#include "user_service.h"

namespace server
{
namespace services
{

UserService::UserService(std::shared_ptr<repositories::IUserRepository> userRepo)
    : m_userRepo(std::move(userRepo))
{
    if (!m_userRepo)
    {
        throw std::runtime_error("UserRepository cannot be null");
    }
}

UsersPage UserService::users(int page, int pageSize)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [users, total] = m_userRepo->findAll(page, pageSize);
    // TODO: Здесь можно добавить бизнес-логику (скрытие полей) когда понадобится.
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
            << "createUser: пользователь с логином '" << *user.login << "' уже существует";
        return std::nullopt;
    }

    const std::string hashedPassword = crypto::sha256(password);
    const int64_t newId = m_userRepo->create(user, hashedPassword);
    if (newId > 0)
    {
        return m_userRepo->findById(newId);
    }
    return std::nullopt;
}

std::optional<dto::User> UserService::updateUser(const dto::User& user)
{
    if (!user.id.has_value())
    {
        LOG_WARN << "updateUser: отсутствует ID";
        return std::nullopt;
    }

    if (!m_userRepo->update(user))
    {
        return std::nullopt;
    }
    return m_userRepo->findById(*user.id);
}

bool UserService::deleteUser(int64_t id)
{
    return m_userRepo->remove(id);
}

} // namespace services
} // namespace server
