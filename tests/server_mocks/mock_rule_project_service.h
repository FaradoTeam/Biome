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
    RuleProjectsPage getRuleProjects(int page, int pageSize, std::optional<int64_t> ruleId = std::nullopt, std::optional<int64_t> projectId = std::nullopt) override
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

    std::optional<dto::RuleProject> createRuleProject(const dto::RuleProject& rp) override
    {
        m_lastCreatedRuleProject = rp;
        ++m_createRuleProjectCallCount;
        return m_createRuleProjectResult;
    }

    std::optional<dto::RuleProject> updateRuleProject(const dto::RuleProject& rp) override
    {
        m_lastUpdatedRuleProject = rp;
        ++m_updateRuleProjectCallCount;
        return m_updateRuleProjectResult;
    }

    bool deleteRuleProject(int64_t id) override
    {
        m_lastDeletedRuleProjectId = id;
        ++m_deleteRuleProjectCallCount;
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
    const dto::RuleProject& getLastUpdatedRuleProject() const { return m_lastUpdatedRuleProject; }
    int64_t getLastDeletedRuleProjectId() const { return m_lastDeletedRuleProjectId; }

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
        m_lastUpdatedRuleProject = dto::RuleProject {};
        m_lastDeletedRuleProjectId = 0;
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
    dto::RuleProject m_lastUpdatedRuleProject;
    int64_t m_lastDeletedRuleProjectId = 0;
};

} // namespace server::tests
