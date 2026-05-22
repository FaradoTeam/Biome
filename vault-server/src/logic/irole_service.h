#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/role.h"

namespace server::services
{

struct RolesPage
{
    std::vector<dto::Role> roles;
    int64_t totalCount = 0;
};

class IRoleService
{
public:
    virtual ~IRoleService() = default;

    virtual RolesPage getRoles(int page, int pageSize, const std::string& searchCaption = "") = 0;
    virtual std::optional<dto::Role> getRole(int64_t id) = 0;
    virtual std::optional<dto::Role> createRole(const dto::Role& role) = 0;
    virtual std::optional<dto::Role> updateRole(const dto::Role& role) = 0;
    virtual bool deleteRole(int64_t id) = 0;
};

} // namespace server::services
