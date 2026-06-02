#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/edge.h"

#include "logic/iedge_service.h"

namespace server
{
namespace tests
{

class MockEdgeService : public services::IEdgeService
{
public:
    using EdgesPage = services::EdgesPage;
    using EdgeResult = services::EdgeResult;

    MockEdgeService()
    {
        // Инициализация с значениями по умолчанию
        m_edgesResult.edges = {};
        m_edgesResult.totalCount = 0;
        m_deleteEdgeResult.success = true;
        m_validateEdgeResult.success = true;
    }

    // Настройка результатов
    void setEdgesResult(const EdgesPage& result)
    {
        m_edgesResult = result;
        m_edgesCallback = nullptr;
    }

    void setEdgeResult(std::optional<dto::Edge> edge)
    {
        m_edgeResult = std::move(edge);
        m_edgeCallback = nullptr;
    }

    void setCreateEdgeResult(std::optional<dto::Edge> edge)
    {
        m_createEdgeResult = std::move(edge);
        m_createEdgeCallback = nullptr;
    }

    void setDeleteEdgeResult(const EdgeResult& result)
    {
        m_deleteEdgeResult = result;
        m_deleteEdgeCallback = nullptr;
    }

    void setWorkflowEdgesResult(const std::vector<dto::Edge>& edges)
    {
        m_workflowEdgesResult = edges;
        m_workflowEdgesCallback = nullptr;
    }

    void setValidateEdgeResult(const EdgeResult& result)
    {
        m_validateEdgeResult = result;
        m_validateEdgeCallback = nullptr;
    }

    // Callback-и для кастомной логики
    void setEdgesCallback(
        std::function<EdgesPage(int, int, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_edgesCallback = std::move(callback);
    }

    void setEdgeCallback(
        std::function<std::optional<dto::Edge>(int64_t)> callback
    )
    {
        m_edgeCallback = std::move(callback);
    }

    void setCreateEdgeCallback(
        std::function<std::optional<dto::Edge>(const dto::Edge&, int64_t)> callback
    )
    {
        m_createEdgeCallback = std::move(callback);
    }

    void setDeleteEdgeCallback(
        std::function<EdgeResult(int64_t, int64_t)> callback
    )
    {
        m_deleteEdgeCallback = std::move(callback);
    }

    void setWorkflowEdgesCallback(
        std::function<std::vector<dto::Edge>(int64_t)> callback
    )
    {
        m_workflowEdgesCallback = std::move(callback);
    }

    // Реализация интерфейса
    EdgesPage edges(
        int page, int pageSize,
        std::optional<int64_t> beginStateId = std::nullopt,
        std::optional<int64_t> endStateId = std::nullopt
    ) override
    {
        m_lastEdgesPage = page;
        m_lastEdgesPageSize = pageSize;
        m_lastEdgesBeginStateId = beginStateId;
        m_lastEdgesEndStateId = endStateId;
        m_edgesCallCount++;

        if (m_edgesCallback)
        {
            return m_edgesCallback(page, pageSize, beginStateId, endStateId);
        }

        // Фильтрация результатов если указаны фильтры
        if (beginStateId.has_value() || endStateId.has_value())
        {
            EdgesPage filtered;
            for (const auto& edge : m_edgesResult.edges)
            {
                bool match = true;
                if (beginStateId.has_value() && edge.beginStateId != beginStateId)
                    match = false;
                if (endStateId.has_value() && edge.endStateId != endStateId)
                    match = false;
                if (match)
                    filtered.edges.push_back(edge);
            }
            filtered.totalCount = filtered.edges.size();
            return filtered;
        }

        return m_edgesResult;
    }

    std::optional<dto::Edge> edge(int64_t id) override
    {
        m_lastEdgeId = id;
        m_edgeCallCount++;

        if (m_edgeCallback)
        {
            return m_edgeCallback(id);
        }
        return m_edgeResult;
    }

    std::optional<dto::Edge> createEdge(
        const dto::Edge& edge,
        int64_t userId
    ) override
    {
        m_lastCreatedEdge = edge;
        m_lastCreateEdgeUserId = userId;
        m_createEdgeCallCount++;

        if (m_createEdgeCallback)
        {
            return m_createEdgeCallback(edge, userId);
        }

        // Симуляция проверки прав: только пользователь 100 может создавать
        if (userId != 100)
        {
            return std::nullopt;
        }
        return m_createEdgeResult;
    }

    EdgeResult deleteEdge(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedEdgeId = id;
        m_lastDeleteEdgeUserId = userId;
        m_deleteEdgeCallCount++;

        if (m_deleteEdgeCallback)
        {
            return m_deleteEdgeCallback(id, userId);
        }

        // Симуляция проверки прав: только пользователь 100 может удалять
        if (userId != 100)
        {
            EdgeResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }
        return m_deleteEdgeResult;
    }

    std::vector<dto::Edge> getWorkflowEdges(int64_t workflowId) override
    {
        m_lastWorkflowEdgesId = workflowId;
        m_workflowEdgesCallCount++;

        if (m_workflowEdgesCallback)
        {
            return m_workflowEdgesCallback(workflowId);
        }
        return m_workflowEdgesResult;
    }

    EdgeResult validateEdge(int64_t beginStateId, int64_t endStateId) override
    {
        m_lastValidateBeginStateId = beginStateId;
        m_lastValidateEndStateId = endStateId;
        m_validateEdgeCallCount++;

        if (m_validateEdgeCallback)
        {
            return m_validateEdgeCallback(beginStateId, endStateId);
        }
        return m_validateEdgeResult;
    }

    // Методы для проверки вызовов
    int getEdgesCallCount() const { return m_edgesCallCount; }
    int getEdgeCallCount() const { return m_edgeCallCount; }
    int getCreateEdgeCallCount() const { return m_createEdgeCallCount; }
    int getDeleteEdgeCallCount() const { return m_deleteEdgeCallCount; }
    int getWorkflowEdgesCallCount() const { return m_workflowEdgesCallCount; }
    int getValidateEdgeCallCount() const { return m_validateEdgeCallCount; }

    int getLastEdgesPage() const { return m_lastEdgesPage; }
    int getLastEdgesPageSize() const { return m_lastEdgesPageSize; }
    std::optional<int64_t> getLastEdgesBeginStateId() const { return m_lastEdgesBeginStateId; }
    std::optional<int64_t> getLastEdgesEndStateId() const { return m_lastEdgesEndStateId; }
    int64_t getLastEdgeId() const { return m_lastEdgeId; }
    const dto::Edge& getLastCreatedEdge() const { return m_lastCreatedEdge; }
    int64_t getLastCreateEdgeUserId() const { return m_lastCreateEdgeUserId; }
    int64_t getLastDeletedEdgeId() const { return m_lastDeletedEdgeId; }
    int64_t getLastDeleteEdgeUserId() const { return m_lastDeleteEdgeUserId; }
    int64_t getLastWorkflowEdgesId() const { return m_lastWorkflowEdgesId; }

    void reset()
    {
        m_edgesCallCount = 0;
        m_edgeCallCount = 0;
        m_createEdgeCallCount = 0;
        m_deleteEdgeCallCount = 0;
        m_workflowEdgesCallCount = 0;
        m_validateEdgeCallCount = 0;
        m_lastEdgesPage = 0;
        m_lastEdgesPageSize = 0;
        m_lastEdgesBeginStateId = std::nullopt;
        m_lastEdgesEndStateId = std::nullopt;
        m_lastEdgeId = 0;
        m_lastCreatedEdge = dto::Edge {};
        m_lastCreateEdgeUserId = 0;
        m_lastDeletedEdgeId = 0;
        m_lastDeleteEdgeUserId = 0;
        m_lastWorkflowEdgesId = 0;
        m_lastValidateBeginStateId = 0;
        m_lastValidateEndStateId = 0;

        // Сброс результатов
        m_edgesResult.edges = {};
        m_edgesResult.totalCount = 0;
        m_edgeResult = std::nullopt;
        m_createEdgeResult = std::nullopt;
        m_deleteEdgeResult.success = true;
        m_workflowEdgesResult.clear();
    }

private:
    EdgesPage m_edgesResult;
    std::optional<dto::Edge> m_edgeResult;
    std::optional<dto::Edge> m_createEdgeResult;
    EdgeResult m_deleteEdgeResult;
    std::vector<dto::Edge> m_workflowEdgesResult;
    EdgeResult m_validateEdgeResult;

    // Callback-и
    std::function<EdgesPage(int, int, std::optional<int64_t>, std::optional<int64_t>)> m_edgesCallback;
    std::function<std::optional<dto::Edge>(int64_t)> m_edgeCallback;
    std::function<std::optional<dto::Edge>(const dto::Edge&, int64_t)> m_createEdgeCallback;
    std::function<EdgeResult(int64_t, int64_t)> m_deleteEdgeCallback;
    std::function<std::vector<dto::Edge>(int64_t)> m_workflowEdgesCallback;
    std::function<EdgeResult(int64_t, int64_t)> m_validateEdgeCallback;

    int m_edgesCallCount = 0;
    int m_edgeCallCount = 0;
    int m_createEdgeCallCount = 0;
    int m_deleteEdgeCallCount = 0;
    int m_workflowEdgesCallCount = 0;
    int m_validateEdgeCallCount = 0;

    int m_lastEdgesPage = 0;
    int m_lastEdgesPageSize = 0;
    std::optional<int64_t> m_lastEdgesBeginStateId;
    std::optional<int64_t> m_lastEdgesEndStateId;
    int64_t m_lastEdgeId = 0;
    dto::Edge m_lastCreatedEdge;
    int64_t m_lastCreateEdgeUserId = 0;
    int64_t m_lastDeletedEdgeId = 0;
    int64_t m_lastDeleteEdgeUserId = 0;
    int64_t m_lastWorkflowEdgesId = 0;
    int64_t m_lastValidateBeginStateId = 0;
    int64_t m_lastValidateEndStateId = 0;
};

} // namespace tests
} // namespace server
