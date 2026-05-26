#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/team.h"

namespace server::services
{

struct TeamsPage
{
    std::vector<dto::Team> teams;
    int64_t totalCount = 0;
};

class ITeamService
{
public:
    virtual ~ITeamService() = default;

    virtual TeamsPage getTeams(int page, int pageSize, const std::string& searchCaption = "") = 0;
    virtual std::optional<dto::Team> getTeam(int64_t id) = 0;
    virtual std::optional<dto::Team> createTeam(const dto::Team& team) = 0;
    virtual std::optional<dto::Team> updateTeam(const dto::Team& team) = 0;
    virtual bool deleteTeam(int64_t id) = 0;
};

} // namespace server::services
