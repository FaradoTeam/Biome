#pragma once

#include <memory>

#include "logic/iteam_service.h"
#include "repo/team_repository.h"

namespace server::services
{

class TeamService final : public ITeamService
{
public:
    explicit TeamService(std::shared_ptr<repositories::ITeamRepository> teamRepo);

    TeamsPage getTeams(int page, int pageSize, const std::string& searchCaption = "") override;
    std::optional<dto::Team> getTeam(int64_t id) override;
    std::optional<dto::Team> createTeam(const dto::Team& team) override;
    std::optional<dto::Team> updateTeam(const dto::Team& team) override;
    bool deleteTeam(int64_t id) override;

private:
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
};

} // namespace server::services
