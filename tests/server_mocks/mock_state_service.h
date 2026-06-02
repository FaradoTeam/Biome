// tests/server_mocks/mock_state_service.h
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/state.h"

#include "logic/istate_service.h"

namespace server
{
namespace tests
{

class MockStateService : public services::IStateService
{
public:
    using StatesPage = services::StatesPage;
    using StateResult = services::StateResult;

    // Настройка результатов
    void setStatesResult(const StatesPage& result)
    {
        m_statesResult = result;
        m_statesCallback = nullptr;
    }

    void setStateResult(std::optional<dto::State> state)
    {
        m_stateResult = std::move(state);
        m_stateCallback = nullptr;
    }

    void setCreateStateResult(std::optional<dto::State> state)
    {
        m_createStateResult = std::move(state);
        m_createStateCallback = nullptr;
    }

    void setUpdateStateResult(std::optional<dto::State> state)
    {
        m_updateStateResult = std::move(state);
        m_updateStateCallback = nullptr;
    }

    void setDeleteStateResult(const StateResult& result)
    {
        m_deleteStateResult = result;
        m_deleteStateCallback = nullptr;
    }

    void setWorkflowStatesResult(const std::vector<dto::State>& states)
    {
        m_workflowStatesResult = states;
        m_workflowStatesCallback = nullptr;
    }

    void setCanDeleteStateResult(bool result)
    {
        m_canDeleteStateResult = result;
        m_canDeleteStateCallback = nullptr;
    }

    // Callback-и
    void setStatesCallback(
        std::function<StatesPage(int, int, std::optional<int64_t>)> callback
    )
    {
        m_statesCallback = std::move(callback);
    }

    void setStateCallback(
        std::function<std::optional<dto::State>(int64_t)> callback
    )
    {
        m_stateCallback = std::move(callback);
    }

    void setCreateStateCallback(
        std::function<std::optional<dto::State>(const dto::State&, int64_t)> callback
    )
    {
        m_createStateCallback = std::move(callback);
    }

    void setUpdateStateCallback(
        std::function<std::optional<dto::State>(const dto::State&, int64_t)> callback
    )
    {
        m_updateStateCallback = std::move(callback);
    }

    void setDeleteStateCallback(
        std::function<StateResult(int64_t, int64_t)> callback
    )
    {
        m_deleteStateCallback = std::move(callback);
    }

    // Реализация интерфейса
    StatesPage states(
        int page, int pageSize,
        std::optional<int64_t> workflowId = std::nullopt
    ) override
    {
        m_lastStatesPage = page;
        m_lastStatesPageSize = pageSize;
        m_lastStatesWorkflowId = workflowId;
        m_statesCallCount++;

        if (m_statesCallback)
        {
            return m_statesCallback(page, pageSize, workflowId);
        }
        return m_statesResult;
    }

    std::optional<dto::State> state(int64_t id) override
    {
        m_lastStateId = id;
        m_stateCallCount++;

        if (m_stateCallback)
        {
            return m_stateCallback(id);
        }
        return m_stateResult;
    }

    std::optional<dto::State> createState(
        const dto::State& state,
        int64_t userId
    ) override
    {
        m_lastCreatedState = state;
        m_lastCreateStateUserId = userId;
        m_createStateCallCount++;

        if (m_createStateCallback)
        {
            return m_createStateCallback(state, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createStateResult;
    }

    std::optional<dto::State> updateState(
        const dto::State& state,
        int64_t userId
    ) override
    {
        m_lastUpdatedState = state;
        m_lastUpdateStateUserId = userId;
        m_updateStateCallCount++;

        if (m_updateStateCallback)
        {
            return m_updateStateCallback(state, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateStateResult;
    }

    StateResult deleteState(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedStateId = id;
        m_lastDeleteStateUserId = userId;
        m_deleteStateCallCount++;

        if (m_deleteStateCallback)
        {
            return m_deleteStateCallback(id, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            StateResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }
        return m_deleteStateResult;
    }

    std::vector<dto::State> getWorkflowStates(int64_t workflowId) override
    {
        m_lastWorkflowStatesId = workflowId;
        m_workflowStatesCallCount++;

        if (m_workflowStatesCallback)
        {
            return m_workflowStatesCallback(workflowId);
        }
        return m_workflowStatesResult;
    }

    bool canDeleteState(int64_t id) override
    {
        m_lastCanDeleteStateId = id;
        m_canDeleteStateCallCount++;

        if (m_canDeleteStateCallback)
        {
            return m_canDeleteStateCallback(id);
        }
        return m_canDeleteStateResult;
    }

    // Методы для проверки вызовов
    int getStatesCallCount() const { return m_statesCallCount; }
    int getStateCallCount() const { return m_stateCallCount; }
    int getCreateStateCallCount() const { return m_createStateCallCount; }
    int getUpdateStateCallCount() const { return m_updateStateCallCount; }
    int getDeleteStateCallCount() const { return m_deleteStateCallCount; }

    int getLastStatesPage() const { return m_lastStatesPage; }
    int getLastStatesPageSize() const { return m_lastStatesPageSize; }
    std::optional<int64_t> getLastStatesWorkflowId() const { return m_lastStatesWorkflowId; }
    int64_t getLastStateId() const { return m_lastStateId; }
    const dto::State& getLastCreatedState() const { return m_lastCreatedState; }
    int64_t getLastCreateStateUserId() const { return m_lastCreateStateUserId; }
    const dto::State& getLastUpdatedState() const { return m_lastUpdatedState; }
    int64_t getLastUpdateStateUserId() const { return m_lastUpdateStateUserId; }
    int64_t getLastDeletedStateId() const { return m_lastDeletedStateId; }
    int64_t getLastDeleteStateUserId() const { return m_lastDeleteStateUserId; }

    void reset()
    {
        m_statesCallCount = 0;
        m_stateCallCount = 0;
        m_createStateCallCount = 0;
        m_updateStateCallCount = 0;
        m_deleteStateCallCount = 0;
        m_workflowStatesCallCount = 0;
        m_canDeleteStateCallCount = 0;
        m_lastStatesPage = 0;
        m_lastStatesPageSize = 0;
        m_lastStatesWorkflowId = std::nullopt;
        m_lastStateId = 0;
        m_lastCreatedState = dto::State {};
        m_lastCreateStateUserId = 0;
        m_lastUpdatedState = dto::State {};
        m_lastUpdateStateUserId = 0;
        m_lastDeletedStateId = 0;
        m_lastDeleteStateUserId = 0;
        m_lastWorkflowStatesId = 0;
    }

private:
    StatesPage m_statesResult;
    std::optional<dto::State> m_stateResult;
    std::optional<dto::State> m_createStateResult;
    std::optional<dto::State> m_updateStateResult;
    StateResult m_deleteStateResult;
    std::vector<dto::State> m_workflowStatesResult;
    bool m_canDeleteStateResult = false;

    // Callback-и
    std::function<StatesPage(int, int, std::optional<int64_t>)> m_statesCallback;
    std::function<std::optional<dto::State>(int64_t)> m_stateCallback;
    std::function<std::optional<dto::State>(const dto::State&, int64_t)> m_createStateCallback;
    std::function<std::optional<dto::State>(const dto::State&, int64_t)> m_updateStateCallback;
    std::function<StateResult(int64_t, int64_t)> m_deleteStateCallback;
    std::function<std::vector<dto::State>(int64_t)> m_workflowStatesCallback;
    std::function<bool(int64_t)> m_canDeleteStateCallback;

    int m_statesCallCount = 0;
    int m_stateCallCount = 0;
    int m_createStateCallCount = 0;
    int m_updateStateCallCount = 0;
    int m_deleteStateCallCount = 0;
    int m_workflowStatesCallCount = 0;
    int m_canDeleteStateCallCount = 0;

    int m_lastStatesPage = 0;
    int m_lastStatesPageSize = 0;
    std::optional<int64_t> m_lastStatesWorkflowId;
    int64_t m_lastStateId = 0;
    dto::State m_lastCreatedState;
    int64_t m_lastCreateStateUserId = 0;
    dto::State m_lastUpdatedState;
    int64_t m_lastUpdateStateUserId = 0;
    int64_t m_lastDeletedStateId = 0;
    int64_t m_lastDeleteStateUserId = 0;
    int64_t m_lastWorkflowStatesId = 0;
    int64_t m_lastCanDeleteStateId = 0;
};

} // namespace tests
} // namespace server
