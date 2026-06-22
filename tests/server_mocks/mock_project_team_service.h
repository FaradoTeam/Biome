#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/dto/project_team.h"
#include "logic/iproject_team_service.h"

namespace server::tests
{

class MockProjectTeamService : public services::IProjectTeamService
{
public:
    using ProjectTeamsPage = services::ProjectTeamsPage;
    using ProjectTeamResult = services::ProjectTeamResult;

    void setGetProjectTeamsResult(const ProjectTeamsPage& result)
    {
        m_getProjectTeamsResult = result;
    }

    void setGetProjectTeamResult(std::optional<dto::ProjectTeam> item)
    {
        m_getProjectTeamResult = std::move(item);
    }

    void setCreateProjectTeamResult(std::optional<dto::ProjectTeam> item)
    {
        m_createProjectTeamResult = item;
    }

    void setCreateProjectTeamResultForUser(
        int64_t userId,
        std::optional<dto::ProjectTeam> item
    )
    {
        m_createProjectTeamResultForUser[userId] = item;
    }

    void setDeleteProjectTeamResult(const ProjectTeamResult& result)
    {
        m_deleteProjectTeamResult = result;
    }

    // Реализация интерфейса
    ProjectTeamsPage getProjectTeams(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt
    ) override
    {
        m_lastGetProjectTeamsPage = page;
        m_lastGetProjectTeamsPageSize = pageSize;
        m_lastGetProjectTeamsProjectId = projectId;
        m_lastGetProjectTeamsTeamId = teamId;
        // Сохраняем userId из последнего вызова
        m_lastGetProjectTeamsUserId = m_lastUserId;
        ++m_getProjectTeamsCallCount;
        return m_getProjectTeamsResult;
    }

    std::optional<dto::ProjectTeam> getProjectTeam(int64_t id) override
    {
        m_lastGetProjectTeamId = id;
        m_lastGetProjectTeamUserId = m_lastUserId;
        ++m_getProjectTeamCallCount;
        return m_getProjectTeamResult;
    }

    std::optional<dto::ProjectTeam> createProjectTeam(
        const dto::ProjectTeam& projectTeam,
        int64_t userId
    ) override
    {
        m_lastCreatedProjectTeam = projectTeam;
        m_lastCreateProjectTeamUserId = userId;
        m_lastUserId = userId;
        ++m_createProjectTeamCallCount;

        // Сначала проверяем права (должны быть до проверки дубликата)
        // Только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }

        // Проверяем, есть ли специальный результат для этого пользователя
        auto it = m_createProjectTeamResultForUser.find(userId);
        if (it != m_createProjectTeamResultForUser.end())
        {
            return it->second;
        }

        // Симуляция дубликата: если пара (projectId, teamId) уже существует
        // Проверяем по сохранённым данным
        for (const auto& item : m_getProjectTeamsResult.items)
        {
            if (*item.projectId == *projectTeam.projectId && *item.teamId == *projectTeam.teamId)
            {
                return std::nullopt; // Дубликат
            }
        }

        return m_createProjectTeamResult;
    }

    ProjectTeamResult deleteProjectTeam(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedProjectTeamId = id;
        m_lastDeleteProjectTeamUserId = userId;
        m_lastUserId = userId;
        ++m_deleteProjectTeamCallCount;

        // Только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            ProjectTeamResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        return m_deleteProjectTeamResult;
    }

    // Геттеры для проверки вызовов
    int getGetProjectTeamsCallCount() const
    {
        return m_getProjectTeamsCallCount;
    }
    int getGetProjectTeamCallCount() const
    {
        return m_getProjectTeamCallCount;
    }
    int getCreateProjectTeamCallCount() const
    {
        return m_createProjectTeamCallCount;
    }
    int getDeleteProjectTeamCallCount() const
    {
        return m_deleteProjectTeamCallCount;
    }

    // Геттеры для параметров последних вызовов
    int getLastGetProjectTeamsPage() const
    {
        return m_lastGetProjectTeamsPage;
    }
    int getLastGetProjectTeamsPageSize() const
    {
        return m_lastGetProjectTeamsPageSize;
    }
    std::optional<int64_t> getLastGetProjectTeamsProjectId() const
    {
        return m_lastGetProjectTeamsProjectId;
    }
    std::optional<int64_t> getLastGetProjectTeamsTeamId() const
    {
        return m_lastGetProjectTeamsTeamId;
    }
    int64_t getLastGetProjectTeamsUserId() const
    {
        return m_lastGetProjectTeamsUserId;
    }
    int64_t getLastGetProjectTeamId() const
    {
        return m_lastGetProjectTeamId;
    }
    int64_t getLastGetProjectTeamUserId() const
    {
        return m_lastGetProjectTeamUserId;
    }
    const dto::ProjectTeam& getLastCreatedProjectTeam() const
    {
        return m_lastCreatedProjectTeam;
    }
    int64_t getLastCreateProjectTeamUserId() const
    {
        return m_lastCreateProjectTeamUserId;
    }
    int64_t getLastDeletedProjectTeamId() const
    {
        return m_lastDeletedProjectTeamId;
    }
    int64_t getLastDeleteProjectTeamUserId() const
    {
        return m_lastDeleteProjectTeamUserId;
    }

    void reset()
    {
        m_getProjectTeamsCallCount = 0;
        m_getProjectTeamCallCount = 0;
        m_createProjectTeamCallCount = 0;
        m_deleteProjectTeamCallCount = 0;
        m_lastGetProjectTeamsPage = 0;
        m_lastGetProjectTeamsPageSize = 0;
        m_lastGetProjectTeamsProjectId = std::nullopt;
        m_lastGetProjectTeamsTeamId = std::nullopt;
        m_lastGetProjectTeamsUserId = 0;
        m_lastUserId = 0;
        m_lastGetProjectTeamId = 0;
        m_lastGetProjectTeamUserId = 0;
        m_lastCreatedProjectTeam = dto::ProjectTeam {};
        m_lastCreateProjectTeamUserId = 0;
        m_lastDeletedProjectTeamId = 0;
        m_lastDeleteProjectTeamUserId = 0;
        m_createProjectTeamResultForUser.clear();
        m_getProjectTeamsResult = ProjectTeamsPage {};
    }

private:
    ProjectTeamsPage m_getProjectTeamsResult;
    std::optional<dto::ProjectTeam> m_getProjectTeamResult;
    std::optional<dto::ProjectTeam> m_createProjectTeamResult;
    std::unordered_map<int64_t, std::optional<dto::ProjectTeam>>
        m_createProjectTeamResultForUser;
    ProjectTeamResult m_deleteProjectTeamResult;

    // Счётчики вызовов
    int m_getProjectTeamsCallCount = 0;
    int m_getProjectTeamCallCount = 0;
    int m_createProjectTeamCallCount = 0;
    int m_deleteProjectTeamCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetProjectTeamsPage = 0;
    int m_lastGetProjectTeamsPageSize = 0;
    std::optional<int64_t> m_lastGetProjectTeamsProjectId;
    std::optional<int64_t> m_lastGetProjectTeamsTeamId;
    int64_t m_lastGetProjectTeamsUserId = 0;
    int64_t m_lastUserId = 0;
    int64_t m_lastGetProjectTeamId = 0;
    int64_t m_lastGetProjectTeamUserId = 0;
    dto::ProjectTeam m_lastCreatedProjectTeam;
    int64_t m_lastCreateProjectTeamUserId = 0;
    int64_t m_lastDeletedProjectTeamId = 0;
    int64_t m_lastDeleteProjectTeamUserId = 0;
};

} // namespace server::tests
