#pragma once

#include <memory>

#include "logic/iuser_service.h"
#include "repo/user_repository.h"

namespace server
{
namespace services
{

class UserService final : public IUserService
{
public:
    explicit UserService(std::shared_ptr<repositories::IUserRepository> userRepo);

    UsersPage users(int page, int pageSize) override;
    std::optional<dto::User> user(int64_t id) override;
    std::optional<dto::User> createUser(const dto::User& user, const std::string& password) override;
    std::optional<dto::User> updateUser(const dto::User& user) override;
    bool deleteUser(int64_t id) override;

private:
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
};

} // namespace services
} // namespace server
