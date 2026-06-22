#pragma once

#include <memory>

#include "../project_team_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

/**
 * @brief SQLite-реализация репозитория для связей проектов и команд.
 */
class SqliteProjectTeamRepository final : public IProjectTeamRepository
{
public:
    explicit SqliteProjectTeamRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteProjectTeamRepository() override = default;

    std::pair<std::vector<dto::ProjectTeam>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt
    ) override;

    std::optional<dto::ProjectTeam> findById(int64_t id) override;
    bool exists(int64_t projectId, int64_t teamId) override;
    int64_t create(const dto::ProjectTeam& projectTeam) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
