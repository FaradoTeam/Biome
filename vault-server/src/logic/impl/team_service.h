#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iteam_service.h"

#include "repo/team_repository.h"

namespace server::services
{

class TeamService final : public ITeamService
{
public:
    TeamService(
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    TeamsPage getTeams(int page, int pageSize, const std::string& searchCaption = "") override;
    std::optional<dto::Team> getTeam(int64_t id) override;
    std::optional<dto::Team> createTeam(const dto::Team& team) override;
    std::optional<dto::Team> updateTeam(const dto::Team& team) override;
    bool deleteTeam(int64_t id) override;

private:
    void invalidateUsersByTeamId(int64_t teamId);

private:
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
