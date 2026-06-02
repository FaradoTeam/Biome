#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/rule_project.h"

#include "logic/irule_project_service.h"

namespace server::tests
{

class MockRuleProjectService : public services::IRuleProjectService
{
public:
    using RuleProjectsPage = services::RuleProjectsPage;

    void setGetRuleProjectsResult(const RuleProjectsPage& result)
    {
        m_getRuleProjectsResult = result;
    }

    void setGetRuleProjectResult(std::optional<dto::RuleProject> rp)
    {
        m_getRuleProjectResult = std::move(rp);
    }

    void setCreateRuleProjectResult(std::optional<dto::RuleProject> rp)
    {
        m_createRuleProjectResult = std::move(rp);
    }

    void setUpdateRuleProjectResult(std::optional<dto::RuleProject> rp)
    {
        m_updateRuleProjectResult = std::move(rp);
    }

    void setDeleteRuleProjectResult(bool result)
    {
        m_deleteRuleProjectResult = result;
    }

    // Реализация интерфейса
    RuleProjectsPage getRuleProjects(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) override
    {
        m_lastGetRuleProjectsPage = page;
        m_lastGetRuleProjectsPageSize = pageSize;
        m_lastGetRuleProjectsRuleId = ruleId;
        m_lastGetRuleProjectsProjectId = projectId;
        ++m_getRuleProjectsCallCount;
        return m_getRuleProjectsResult;
    }

    std::optional<dto::RuleProject> getRuleProject(int64_t id) override
    {
        m_lastGetRuleProjectId = id;
        ++m_getRuleProjectCallCount;
        return m_getRuleProjectResult;
    }

    std::optional<dto::RuleProject> createRuleProject(
        const dto::RuleProject& rp,
        int64_t userId
    ) override
    {
        m_lastCreatedRuleProject = rp;
        m_lastCreateRuleProjectUserId = userId;
        ++m_createRuleProjectCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createRuleProjectResult;
    }

    std::optional<dto::RuleProject> updateRuleProject(
        const dto::RuleProject& rp,
        int64_t userId
    ) override
    {
        m_lastUpdatedRuleProject = rp;
        m_lastUpdateRuleProjectUserId = userId;
        ++m_updateRuleProjectCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateRuleProjectResult;
    }

    bool deleteRuleProject(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedRuleProjectId = id;
        m_lastDeleteRuleProjectUserId = userId;
        ++m_deleteRuleProjectCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteRuleProjectResult;
    }

    // Методы для проверки
    int getGetRuleProjectsCallCount() const { return m_getRuleProjectsCallCount; }
    int getGetRuleProjectCallCount() const { return m_getRuleProjectCallCount; }
    int getCreateRuleProjectCallCount() const { return m_createRuleProjectCallCount; }
    int getUpdateRuleProjectCallCount() const { return m_updateRuleProjectCallCount; }
    int getDeleteRuleProjectCallCount() const { return m_deleteRuleProjectCallCount; }

    int getLastGetRuleProjectsPage() const { return m_lastGetRuleProjectsPage; }
    int getLastGetRuleProjectsPageSize() const { return m_lastGetRuleProjectsPageSize; }
    std::optional<int64_t> getLastGetRuleProjectsRuleId() const { return m_lastGetRuleProjectsRuleId; }
    std::optional<int64_t> getLastGetRuleProjectsProjectId() const { return m_lastGetRuleProjectsProjectId; }
    int64_t getLastGetRuleProjectId() const { return m_lastGetRuleProjectId; }
    const dto::RuleProject& getLastCreatedRuleProject() const { return m_lastCreatedRuleProject; }
    int64_t getLastCreateRuleProjectUserId() const { return m_lastCreateRuleProjectUserId; }
    const dto::RuleProject& getLastUpdatedRuleProject() const { return m_lastUpdatedRuleProject; }
    int64_t getLastUpdateRuleProjectUserId() const { return m_lastUpdateRuleProjectUserId; }
    int64_t getLastDeletedRuleProjectId() const { return m_lastDeletedRuleProjectId; }
    int64_t getLastDeleteRuleProjectUserId() const { return m_lastDeleteRuleProjectUserId; }

    void reset()
    {
        m_getRuleProjectsCallCount = 0;
        m_getRuleProjectCallCount = 0;
        m_createRuleProjectCallCount = 0;
        m_updateRuleProjectCallCount = 0;
        m_deleteRuleProjectCallCount = 0;
        m_lastGetRuleProjectsPage = 0;
        m_lastGetRuleProjectsPageSize = 0;
        m_lastGetRuleProjectsRuleId.reset();
        m_lastGetRuleProjectsProjectId.reset();
        m_lastGetRuleProjectId = 0;
        m_lastCreatedRuleProject = dto::RuleProject {};
        m_lastCreateRuleProjectUserId = 0;
        m_lastUpdatedRuleProject = dto::RuleProject {};
        m_lastUpdateRuleProjectUserId = 0;
        m_lastDeletedRuleProjectId = 0;
        m_lastDeleteRuleProjectUserId = 0;
    }

private:
    RuleProjectsPage m_getRuleProjectsResult;
    std::optional<dto::RuleProject> m_getRuleProjectResult;
    std::optional<dto::RuleProject> m_createRuleProjectResult;
    std::optional<dto::RuleProject> m_updateRuleProjectResult;
    bool m_deleteRuleProjectResult = false;

    int m_getRuleProjectsCallCount = 0;
    int m_getRuleProjectCallCount = 0;
    int m_createRuleProjectCallCount = 0;
    int m_updateRuleProjectCallCount = 0;
    int m_deleteRuleProjectCallCount = 0;

    int m_lastGetRuleProjectsPage = 0;
    int m_lastGetRuleProjectsPageSize = 0;
    std::optional<int64_t> m_lastGetRuleProjectsRuleId;
    std::optional<int64_t> m_lastGetRuleProjectsProjectId;
    int64_t m_lastGetRuleProjectId = 0;
    dto::RuleProject m_lastCreatedRuleProject;
    int64_t m_lastCreateRuleProjectUserId = 0;
    dto::RuleProject m_lastUpdatedRuleProject;
    int64_t m_lastUpdateRuleProjectUserId = 0;
    int64_t m_lastDeletedRuleProjectId = 0;
    int64_t m_lastDeleteRuleProjectUserId = 0;
};

} // namespace server::tests
