#pragma once

#include <string>

#include "common/dto/user.h"

namespace server
{
namespace services
{

struct UsersPage
{
    std::vector<dto::User> users;
    int64_t totalCount = 0;
};

class IUserService
{
public:
    virtual ~IUserService() = default;
    virtual UsersPage users(int page, int pageSize) = 0;
    virtual std::optional<dto::User> user(int64_t id) = 0;
    virtual std::optional<dto::User> createUser(const dto::User& user, const std::string& password) = 0;
    virtual std::optional<dto::User> updateUser(const dto::User& user) = 0;
    virtual bool deleteUser(int64_t id) = 0;
};

} // namespace services
} // namespace server
