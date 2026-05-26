#pragma once

#include <memory>

#include "../user_team_role_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteUserTeamRoleRepository final : public IUserTeamRoleRepository
{
public:
    explicit SqliteUserTeamRoleRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserTeamRoleRepository() override = default;

    std::pair<std::vector<dto::UserTeamRole>, int64_t> findAll(
        int page, int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) override;
    std::optional<dto::UserTeamRole> findById(int64_t id) override;
    std::vector<dto::UserTeamRole> findByUserId(int64_t userId) override;
    std::vector<dto::UserTeamRole> findByTeamId(int64_t teamId) override;
    std::optional<dto::UserTeamRole> findByUserAndTeam(int64_t userId, int64_t teamId) override;
    bool exists(int64_t userId, int64_t teamId) override;
    int64_t create(const dto::UserTeamRole& userTeamRole) override;
    bool update(const dto::UserTeamRole& userTeamRole) override;
    bool remove(int64_t id) override;
    bool removeByUserAndTeam(int64_t userId, int64_t teamId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
