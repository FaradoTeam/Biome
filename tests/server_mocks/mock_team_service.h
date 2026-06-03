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
        m_getTeamsCallback = nullptr;
    }

    void setGetTeamResult(std::optional<dto::Team> team)
    {
        m_getTeamResult = std::move(team);
        m_getTeamCallback = nullptr;
    }

    void setCreateTeamResult(std::optional<dto::Team> team)
    {
        m_createTeamResult = std::move(team);
        m_createTeamCallback = nullptr;
    }

    void setUpdateTeamResult(std::optional<dto::Team> team)
    {
        m_updateTeamResult = std::move(team);
        m_updateTeamCallback = nullptr;
    }

    void setDeleteTeamResult(bool result)
    {
        m_deleteTeamResult = result;
        m_deleteTeamCallback = nullptr;
    }

    // Callback-и для кастомной логики
    void setGetTeamsCallback(
        std::function<TeamsPage(int, int, int64_t, const std::string&)> callback
    )
    {
        m_getTeamsCallback = std::move(callback);
    }

    void setGetTeamCallback(
        std::function<std::optional<dto::Team>(int64_t, int64_t)> callback
    )
    {
        m_getTeamCallback = std::move(callback);
    }

    void setCreateTeamCallback(
        std::function<std::optional<dto::Team>(const dto::Team&, int64_t)> callback
    )
    {
        m_createTeamCallback = std::move(callback);
    }

    void setUpdateTeamCallback(
        std::function<std::optional<dto::Team>(const dto::Team&, int64_t)> callback
    )
    {
        m_updateTeamCallback = std::move(callback);
    }

    void setDeleteTeamCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteTeamCallback = std::move(callback);
    }

    // Реализация интерфейса ITeamService
    services::TeamsPage getTeams(
        int page,
        int pageSize,
        int64_t userId,
        const std::string& searchCaption = ""
    ) override
    {
        m_lastGetTeamsPage = page;
        m_lastGetTeamsPageSize = pageSize;
        m_lastGetTeamsUserId = userId;
        m_lastGetTeamsSearch = searchCaption;
        ++m_getTeamsCallCount;

        if (m_getTeamsCallback)
        {
            return m_getTeamsCallback(page, pageSize, userId, searchCaption);
        }

        // Только супер-админ (userId=1) может просматривать список команд
        if (userId != 1)
        {
            services::TeamsPage emptyPage;
            emptyPage.totalCount = 0;
            return emptyPage;
        }

        return m_getTeamsResult;
    }

    std::optional<dto::Team> getTeam(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastGetTeamId = id;
        m_lastGetTeamUserId = userId;
        ++m_getTeamCallCount;

        if (m_getTeamCallback)
        {
            return m_getTeamCallback(id, userId);
        }

        // Только супер-админ (userId=1) может просматривать команду
        if (userId != 1)
        {
            return std::nullopt;
        }

        if (m_getTeamResult.has_value() && m_getTeamResult->id.has_value())
        {
            if (*m_getTeamResult->id == id)
            {
                return m_getTeamResult;
            }
        }
        return std::nullopt;
    }

    std::optional<dto::Team> createTeam(
        const dto::Team& team,
        int64_t userId
    ) override
    {
        m_lastCreatedTeam = team;
        m_lastCreateTeamUserId = userId;
        ++m_createTeamCallCount;

        if (m_createTeamCallback)
        {
            return m_createTeamCallback(team, userId);
        }

        // Только супер-админ (userId=1) может создавать команды
        if (userId != 1)
        {
            return std::nullopt;
        }

        return m_createTeamResult;
    }

    std::optional<dto::Team> updateTeam(
        const dto::Team& team,
        int64_t userId
    ) override
    {
        m_lastUpdatedTeam = team;
        m_lastUpdateTeamUserId = userId;
        ++m_updateTeamCallCount;

        if (m_updateTeamCallback)
        {
            return m_updateTeamCallback(team, userId);
        }

        // Только супер-админ (userId=1) может обновлять команды
        if (userId != 1)
        {
            return std::nullopt;
        }

        return m_updateTeamResult;
    }

    bool deleteTeam(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedTeamId = id;
        m_lastDeleteTeamUserId = userId;
        ++m_deleteTeamCallCount;

        if (m_deleteTeamCallback)
        {
            return m_deleteTeamCallback(id, userId);
        }

        // Только супер-админ (userId=1) может удалять команды
        if (userId != 1)
        {
            return false;
        }

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
    int64_t getLastGetTeamsUserId() const { return m_lastGetTeamsUserId; }
    const std::string& getLastGetTeamsSearch() const { return m_lastGetTeamsSearch; }
    int64_t getLastGetTeamId() const { return m_lastGetTeamId; }
    int64_t getLastGetTeamUserId() const { return m_lastGetTeamUserId; }
    const dto::Team& getLastCreatedTeam() const { return m_lastCreatedTeam; }
    int64_t getLastCreateTeamUserId() const { return m_lastCreateTeamUserId; }
    const dto::Team& getLastUpdatedTeam() const { return m_lastUpdatedTeam; }
    int64_t getLastUpdateTeamUserId() const { return m_lastUpdateTeamUserId; }
    int64_t getLastDeletedTeamId() const { return m_lastDeletedTeamId; }
    int64_t getLastDeleteTeamUserId() const { return m_lastDeleteTeamUserId; }

    void reset()
    {
        m_getTeamsCallCount = 0;
        m_getTeamCallCount = 0;
        m_createTeamCallCount = 0;
        m_updateTeamCallCount = 0;
        m_deleteTeamCallCount = 0;

        m_lastGetTeamsPage = 0;
        m_lastGetTeamsPageSize = 0;
        m_lastGetTeamsUserId = 0;
        m_lastGetTeamsSearch.clear();
        m_lastGetTeamId = 0;
        m_lastGetTeamUserId = 0;
        m_lastCreatedTeam = dto::Team {};
        m_lastCreateTeamUserId = 0;
        m_lastUpdatedTeam = dto::Team {};
        m_lastUpdateTeamUserId = 0;
        m_lastDeletedTeamId = 0;
        m_lastDeleteTeamUserId = 0;

        m_getTeamsCallback = nullptr;
        m_getTeamCallback = nullptr;
        m_createTeamCallback = nullptr;
        m_updateTeamCallback = nullptr;
        m_deleteTeamCallback = nullptr;

        m_getTeamsResult = services::TeamsPage {};
        m_getTeamResult = std::nullopt;
        m_createTeamResult = std::nullopt;
        m_updateTeamResult = std::nullopt;
        m_deleteTeamResult = false;
        m_nextId = 100;
    }

private:
    services::TeamsPage m_getTeamsResult;
    std::optional<dto::Team> m_getTeamResult;
    std::optional<dto::Team> m_createTeamResult;
    std::optional<dto::Team> m_updateTeamResult;
    bool m_deleteTeamResult = false;

    // Callback-и
    std::function<services::TeamsPage(int, int, int64_t, const std::string&)> m_getTeamsCallback;
    std::function<std::optional<dto::Team>(int64_t, int64_t)> m_getTeamCallback;
    std::function<std::optional<dto::Team>(const dto::Team&, int64_t)> m_createTeamCallback;
    std::function<std::optional<dto::Team>(const dto::Team&, int64_t)> m_updateTeamCallback;
    std::function<bool(int64_t, int64_t)> m_deleteTeamCallback;

    // Счётчики вызовов
    int m_getTeamsCallCount = 0;
    int m_getTeamCallCount = 0;
    int m_createTeamCallCount = 0;
    int m_updateTeamCallCount = 0;
    int m_deleteTeamCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetTeamsPage = 0;
    int m_lastGetTeamsPageSize = 0;
    int64_t m_lastGetTeamsUserId = 0;
    std::string m_lastGetTeamsSearch;
    int64_t m_lastGetTeamId = 0;
    int64_t m_lastGetTeamUserId = 0;
    dto::Team m_lastCreatedTeam;
    int64_t m_lastCreateTeamUserId = 0;
    dto::Team m_lastUpdatedTeam;
    int64_t m_lastUpdateTeamUserId = 0;
    int64_t m_lastDeletedTeamId = 0;
    int64_t m_lastDeleteTeamUserId = 0;
    int64_t m_nextId = 100;
};

} // namespace server::tests
