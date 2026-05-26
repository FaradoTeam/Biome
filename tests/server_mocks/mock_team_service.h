#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/team.h"

#include "logic/iteam_service.h"

namespace server::tests
{

class MockTeamService : public services::ITeamService
{
public:
    using TeamsPage = services::TeamsPage;

    void setGetTeamsResult(const TeamsPage& result)
    {
        m_getTeamsResult = result;
    }

    void setGetTeamResult(std::optional<dto::Team> team)
    {
        m_getTeamResult = std::move(team);
    }

    void setCreateTeamResult(std::optional<dto::Team> team)
    {
        m_createTeamResult = std::move(team);
    }

    void setUpdateTeamResult(std::optional<dto::Team> team)
    {
        m_updateTeamResult = std::move(team);
    }

    void setDeleteTeamResult(bool result)
    {
        m_deleteTeamResult = result;
    }

    // Реализация интерфейса
    TeamsPage getTeams(int page, int pageSize, const std::string& searchCaption = "") override
    {
        m_lastGetTeamsPage = page;
        m_lastGetTeamsPageSize = pageSize;
        m_lastGetTeamsSearch = searchCaption;
        ++m_getTeamsCallCount;
        return m_getTeamsResult;
    }

    std::optional<dto::Team> getTeam(int64_t id) override
    {
        m_lastGetTeamId = id;
        ++m_getTeamCallCount;
        return m_getTeamResult;
    }

    std::optional<dto::Team> createTeam(const dto::Team& team) override
    {
        m_lastCreatedTeam = team;
        ++m_createTeamCallCount;
        return m_createTeamResult;
    }

    std::optional<dto::Team> updateTeam(const dto::Team& team) override
    {
        m_lastUpdatedTeam = team;
        ++m_updateTeamCallCount;
        return m_updateTeamResult;
    }

    bool deleteTeam(int64_t id) override
    {
        m_lastDeletedTeamId = id;
        ++m_deleteTeamCallCount;
        return m_deleteTeamResult;
    }

    // Методы для проверки вызовов
    int getGetTeamsCallCount() const { return m_getTeamsCallCount; }
    int getGetTeamCallCount() const { return m_getTeamCallCount; }
    int getCreateTeamCallCount() const { return m_createTeamCallCount; }
    int getUpdateTeamCallCount() const { return m_updateTeamCallCount; }
    int getDeleteTeamCallCount() const { return m_deleteTeamCallCount; }

    int getLastGetTeamsPage() const { return m_lastGetTeamsPage; }
    int getLastGetTeamsPageSize() const { return m_lastGetTeamsPageSize; }
    const std::string& getLastGetTeamsSearch() const { return m_lastGetTeamsSearch; }
    int64_t getLastGetTeamId() const { return m_lastGetTeamId; }
    const dto::Team& getLastCreatedTeam() const { return m_lastCreatedTeam; }
    const dto::Team& getLastUpdatedTeam() const { return m_lastUpdatedTeam; }
    int64_t getLastDeletedTeamId() const { return m_lastDeletedTeamId; }

    void reset()
    {
        m_getTeamsCallCount = 0;
        m_getTeamCallCount = 0;
        m_createTeamCallCount = 0;
        m_updateTeamCallCount = 0;
        m_deleteTeamCallCount = 0;
        m_lastGetTeamsPage = 0;
        m_lastGetTeamsPageSize = 0;
        m_lastGetTeamsSearch.clear();
        m_lastGetTeamId = 0;
        m_lastCreatedTeam = dto::Team {};
        m_lastUpdatedTeam = dto::Team {};
        m_lastDeletedTeamId = 0;
    }

private:
    TeamsPage m_getTeamsResult;
    std::optional<dto::Team> m_getTeamResult;
    std::optional<dto::Team> m_createTeamResult;
    std::optional<dto::Team> m_updateTeamResult;
    bool m_deleteTeamResult = false;

    int m_getTeamsCallCount = 0;
    int m_getTeamCallCount = 0;
    int m_createTeamCallCount = 0;
    int m_updateTeamCallCount = 0;
    int m_deleteTeamCallCount = 0;

    int m_lastGetTeamsPage = 0;
    int m_lastGetTeamsPageSize = 0;
    std::string m_lastGetTeamsSearch;
    int64_t m_lastGetTeamId = 0;
    dto::Team m_lastCreatedTeam;
    dto::Team m_lastUpdatedTeam;
    int64_t m_lastDeletedTeamId = 0;
};

} // namespace server::tests
