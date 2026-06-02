// tests/server_mocks/mock_workflow_service.h
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/workflow.h"

#include "logic/iworkflow_service.h"

namespace server
{
namespace tests
{

class MockWorkflowService : public services::IWorkflowService
{
public:
    using WorkflowsPage = services::WorkflowsPage;
    using WorkflowResult = services::WorkflowResult;

    MockWorkflowService()
    {
        m_workflowsResult.workflows = {};
        m_workflowsResult.totalCount = 0;
        m_deleteWorkflowResult.success = true;
        m_canDeleteWorkflowResult = true;
    }

    // Настройка результатов
    void setWorkflowsResult(const WorkflowsPage& result)
    {
        m_workflowsResult = result;
        m_workflowsCallback = nullptr;
    }

    void setWorkflowResult(std::optional<dto::Workflow> workflow)
    {
        m_workflowResult = std::move(workflow);
        m_workflowCallback = nullptr;
    }

    void setCreateWorkflowResult(std::optional<dto::Workflow> workflow)
    {
        m_createWorkflowResult = std::move(workflow);
        m_createWorkflowCallback = nullptr;
    }

    void setUpdateWorkflowResult(std::optional<dto::Workflow> workflow)
    {
        m_updateWorkflowResult = std::move(workflow);
        m_updateWorkflowCallback = nullptr;
    }

    void setDeleteWorkflowResult(const WorkflowResult& result)
    {
        m_deleteWorkflowResult = result;
        m_deleteWorkflowCallback = nullptr;
    }

    void setCanDeleteWorkflowResult(bool result)
    {
        m_canDeleteWorkflowResult = result;
        m_canDeleteWorkflowCallback = nullptr;
    }

    // Callback-и
    void setWorkflowsCallback(
        std::function<WorkflowsPage(int, int)> callback
    )
    {
        m_workflowsCallback = std::move(callback);
    }

    void setWorkflowCallback(
        std::function<std::optional<dto::Workflow>(int64_t)> callback
    )
    {
        m_workflowCallback = std::move(callback);
    }

    void setCreateWorkflowCallback(
        std::function<std::optional<dto::Workflow>(const dto::Workflow&, int64_t)> callback
    )
    {
        m_createWorkflowCallback = std::move(callback);
    }

    void setUpdateWorkflowCallback(
        std::function<std::optional<dto::Workflow>(const dto::Workflow&, int64_t)> callback
    )
    {
        m_updateWorkflowCallback = std::move(callback);
    }

    void setDeleteWorkflowCallback(
        std::function<WorkflowResult(int64_t, int64_t)> callback
    )
    {
        m_deleteWorkflowCallback = std::move(callback);
    }

    // Реализация интерфейса
    WorkflowsPage workflows(int page, int pageSize) override
    {
        m_lastWorkflowsPage = page;
        m_lastWorkflowsPageSize = pageSize;
        m_workflowsCallCount++;

        if (m_workflowsCallback)
        {
            return m_workflowsCallback(page, pageSize);
        }

        // Пагинация
        WorkflowsPage result;
        int start = (page - 1) * pageSize;
        int end = std::min(start + pageSize, (int)m_workflowsResult.workflows.size());

        if (start < (int)m_workflowsResult.workflows.size())
        {
            result.workflows.assign(
                m_workflowsResult.workflows.begin() + start,
                m_workflowsResult.workflows.begin() + end
            );
        }
        result.totalCount = m_workflowsResult.totalCount;

        return result;
    }

    std::optional<dto::Workflow> workflow(int64_t id) override
    {
        m_lastWorkflowId = id;
        m_workflowCallCount++;

        if (m_workflowCallback)
        {
            return m_workflowCallback(id);
        }

        // Поиск по ID
        for (const auto& wf : m_workflowsResult.workflows)
        {
            if (wf.id == id)
                return wf;
        }

        return m_workflowResult;
    }

    std::optional<dto::Workflow> createWorkflow(
        const dto::Workflow& workflow,
        int64_t userId
    ) override
    {
        m_lastCreatedWorkflow = workflow;
        m_lastCreateWorkflowUserId = userId;
        m_createWorkflowCallCount++;

        if (m_createWorkflowCallback)
        {
            return m_createWorkflowCallback(workflow, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }

        // Проверка на дубликат по caption
        for (const auto& wf : m_workflowsResult.workflows)
        {
            if (wf.caption == workflow.caption)
            {
                return std::nullopt;
            }
        }

        return m_createWorkflowResult;
    }

    std::optional<dto::Workflow> updateWorkflow(
        const dto::Workflow& workflow,
        int64_t userId
    ) override
    {
        m_lastUpdatedWorkflow = workflow;
        m_lastUpdateWorkflowUserId = userId;
        m_updateWorkflowCallCount++;

        if (m_updateWorkflowCallback)
        {
            return m_updateWorkflowCallback(workflow, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateWorkflowResult;
    }

    WorkflowResult deleteWorkflow(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedWorkflowId = id;
        m_lastDeleteWorkflowUserId = userId;
        m_deleteWorkflowCallCount++;

        if (m_deleteWorkflowCallback)
        {
            return m_deleteWorkflowCallback(id, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            WorkflowResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }
        return m_deleteWorkflowResult;
    }

    bool canDeleteWorkflow(int64_t id) override
    {
        m_lastCanDeleteWorkflowId = id;
        m_canDeleteWorkflowCallCount++;

        if (m_canDeleteWorkflowCallback)
        {
            return m_canDeleteWorkflowCallback(id);
        }
        return m_canDeleteWorkflowResult;
    }

    // Методы для проверки вызовов
    int getWorkflowsCallCount() const { return m_workflowsCallCount; }
    int getWorkflowCallCount() const { return m_workflowCallCount; }
    int getCreateWorkflowCallCount() const { return m_createWorkflowCallCount; }
    int getUpdateWorkflowCallCount() const { return m_updateWorkflowCallCount; }
    int getDeleteWorkflowCallCount() const { return m_deleteWorkflowCallCount; }
    int getCanDeleteWorkflowCallCount() const { return m_canDeleteWorkflowCallCount; }

    int getLastWorkflowsPage() const { return m_lastWorkflowsPage; }
    int getLastWorkflowsPageSize() const { return m_lastWorkflowsPageSize; }
    int64_t getLastWorkflowId() const { return m_lastWorkflowId; }
    const dto::Workflow& getLastCreatedWorkflow() const { return m_lastCreatedWorkflow; }
    int64_t getLastCreateWorkflowUserId() const { return m_lastCreateWorkflowUserId; }
    const dto::Workflow& getLastUpdatedWorkflow() const { return m_lastUpdatedWorkflow; }
    int64_t getLastUpdateWorkflowUserId() const { return m_lastUpdateWorkflowUserId; }
    int64_t getLastDeletedWorkflowId() const { return m_lastDeletedWorkflowId; }
    int64_t getLastDeleteWorkflowUserId() const { return m_lastDeleteWorkflowUserId; }

    // Добавляем метод для добавления существующих workflow (для теста дубликата)
    void addWorkflow(const dto::Workflow& workflow)
    {
        m_workflowsResult.workflows.push_back(workflow);
        m_workflowsResult.totalCount = m_workflowsResult.workflows.size();
    }

    void reset()
    {
        m_workflowsCallCount = 0;
        m_workflowCallCount = 0;
        m_createWorkflowCallCount = 0;
        m_updateWorkflowCallCount = 0;
        m_deleteWorkflowCallCount = 0;
        m_canDeleteWorkflowCallCount = 0;
        m_lastWorkflowsPage = 0;
        m_lastWorkflowsPageSize = 0;
        m_lastWorkflowId = 0;
        m_lastCreatedWorkflow = dto::Workflow {};
        m_lastCreateWorkflowUserId = 0;
        m_lastUpdatedWorkflow = dto::Workflow {};
        m_lastUpdateWorkflowUserId = 0;
        m_lastDeletedWorkflowId = 0;
        m_lastDeleteWorkflowUserId = 0;
        m_lastCanDeleteWorkflowId = 0;

        // Сброс результатов
        m_workflowsResult.workflows = {};
        m_workflowsResult.totalCount = 0;
        m_workflowResult = std::nullopt;
        m_createWorkflowResult = std::nullopt;
        m_updateWorkflowResult = std::nullopt;
        m_deleteWorkflowResult.success = true;
        m_canDeleteWorkflowResult = true;
    }

private:
    WorkflowsPage m_workflowsResult;
    std::optional<dto::Workflow> m_workflowResult;
    std::optional<dto::Workflow> m_createWorkflowResult;
    std::optional<dto::Workflow> m_updateWorkflowResult;
    WorkflowResult m_deleteWorkflowResult;
    bool m_canDeleteWorkflowResult = true;

    // Callback-и
    std::function<WorkflowsPage(int, int)> m_workflowsCallback;
    std::function<std::optional<dto::Workflow>(int64_t)> m_workflowCallback;
    std::function<std::optional<dto::Workflow>(const dto::Workflow&, int64_t)> m_createWorkflowCallback;
    std::function<std::optional<dto::Workflow>(const dto::Workflow&, int64_t)> m_updateWorkflowCallback;
    std::function<WorkflowResult(int64_t, int64_t)> m_deleteWorkflowCallback;
    std::function<bool(int64_t)> m_canDeleteWorkflowCallback;

    int m_workflowsCallCount = 0;
    int m_workflowCallCount = 0;
    int m_createWorkflowCallCount = 0;
    int m_updateWorkflowCallCount = 0;
    int m_deleteWorkflowCallCount = 0;
    int m_canDeleteWorkflowCallCount = 0;

    int m_lastWorkflowsPage = 0;
    int m_lastWorkflowsPageSize = 0;
    int64_t m_lastWorkflowId = 0;
    dto::Workflow m_lastCreatedWorkflow;
    int64_t m_lastCreateWorkflowUserId = 0;
    dto::Workflow m_lastUpdatedWorkflow;
    int64_t m_lastUpdateWorkflowUserId = 0;
    int64_t m_lastDeletedWorkflowId = 0;
    int64_t m_lastDeleteWorkflowUserId = 0;
    int64_t m_lastCanDeleteWorkflowId = 0;
};

} // namespace tests
} // namespace server
