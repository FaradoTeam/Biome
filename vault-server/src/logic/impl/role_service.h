#pragma once

#include <memory>

#include "logic/irole_service.h"
#include "repo/role_repository.h"

namespace server::services
{

class RoleService final : public IRoleService
{
public:
    explicit RoleService(std::shared_ptr<repositories::IRoleRepository> roleRepo);

    RolesPage getRoles(int page, int pageSize, const std::string& searchCaption = "") override;
    std::optional<dto::Role> getRole(int64_t id) override;
    std::optional<dto::Role> createRole(const dto::Role& role) override;
    std::optional<dto::Role> updateRole(const dto::Role& role) override;
    bool deleteRole(int64_t id) override;

private:
    std::shared_ptr<repositories::IRoleRepository> m_roleRepo;
};

} // namespace server::services
