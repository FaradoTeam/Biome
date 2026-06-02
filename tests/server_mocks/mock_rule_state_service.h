#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/rule_state.h"

#include "logic/irule_state_service.h"

namespace server::tests
{

class MockRuleStateService : public services::IRuleStateService
{
public:
    using RuleStatesPage = services::RuleStatesPage;

    void setGetRuleStatesResult(const RuleStatesPage& result)
    {
        m_getRuleStatesResult = result;
    }

    void setGetRuleStateResult(std::optional<dto::RuleState> rs)
    {
        m_getRuleStateResult = std::move(rs);
    }

    void setCreateRuleStateResult(std::optional<dto::RuleState> rs)
    {
        m_createRuleStateResult = std::move(rs);
    }

    void setUpdateRuleStateResult(std::optional<dto::RuleState> rs)
    {
        m_updateRuleStateResult = std::move(rs);
    }

    void setDeleteRuleStateResult(bool result)
    {
        m_deleteRuleStateResult = result;
    }

    // Реализация интерфейса
    RuleStatesPage getRuleStates(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override
    {
        m_lastGetRuleStatesPage = page;
        m_lastGetRuleStatesPageSize = pageSize;
        m_lastGetRuleStatesRuleId = ruleId;
        m_lastGetRuleStatesStateId = stateId;
        ++m_getRuleStatesCallCount;
        return m_getRuleStatesResult;
    }

    std::optional<dto::RuleState> getRuleState(int64_t id) override
    {
        m_lastGetRuleStateId = id;
        ++m_getRuleStateCallCount;
        return m_getRuleStateResult;
    }

    std::optional<dto::RuleState> createRuleState(
        const dto::RuleState& rs,
        int64_t userId
    ) override
    {
        m_lastCreatedRuleState = rs;
        m_lastCreateRuleStateUserId = userId;
        ++m_createRuleStateCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createRuleStateResult;
    }

    std::optional<dto::RuleState> updateRuleState(
        const dto::RuleState& rs,
        int64_t userId
    ) override
    {
        m_lastUpdatedRuleState = rs;
        m_lastUpdateRuleStateUserId = userId;
        ++m_updateRuleStateCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateRuleStateResult;
    }

    bool deleteRuleState(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedRuleStateId = id;
        m_lastDeleteRuleStateUserId = userId;
        ++m_deleteRuleStateCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteRuleStateResult;
    }

    // Методы для проверки
    int getGetRuleStatesCallCount() const { return m_getRuleStatesCallCount; }
    int getGetRuleStateCallCount() const { return m_getRuleStateCallCount; }
    int getCreateRuleStateCallCount() const { return m_createRuleStateCallCount; }
    int getUpdateRuleStateCallCount() const { return m_updateRuleStateCallCount; }
    int getDeleteRuleStateCallCount() const { return m_deleteRuleStateCallCount; }

    int getLastGetRuleStatesPage() const { return m_lastGetRuleStatesPage; }
    int getLastGetRuleStatesPageSize() const { return m_lastGetRuleStatesPageSize; }
    std::optional<int64_t> getLastGetRuleStatesRuleId() const { return m_lastGetRuleStatesRuleId; }
    std::optional<int64_t> getLastGetRuleStatesStateId() const { return m_lastGetRuleStatesStateId; }
    int64_t getLastGetRuleStateId() const { return m_lastGetRuleStateId; }
    const dto::RuleState& getLastCreatedRuleState() const { return m_lastCreatedRuleState; }
    int64_t getLastCreateRuleStateUserId() const { return m_lastCreateRuleStateUserId; }
    const dto::RuleState& getLastUpdatedRuleState() const { return m_lastUpdatedRuleState; }
    int64_t getLastUpdateRuleStateUserId() const { return m_lastUpdateRuleStateUserId; }
    int64_t getLastDeletedRuleStateId() const { return m_lastDeletedRuleStateId; }
    int64_t getLastDeleteRuleStateUserId() const { return m_lastDeleteRuleStateUserId; }

    void reset()
    {
        m_getRuleStatesCallCount = 0;
        m_getRuleStateCallCount = 0;
        m_createRuleStateCallCount = 0;
        m_updateRuleStateCallCount = 0;
        m_deleteRuleStateCallCount = 0;
        m_lastGetRuleStatesPage = 0;
        m_lastGetRuleStatesPageSize = 0;
        m_lastGetRuleStatesRuleId.reset();
        m_lastGetRuleStatesStateId.reset();
        m_lastGetRuleStateId = 0;
        m_lastCreatedRuleState = dto::RuleState {};
        m_lastCreateRuleStateUserId = 0;
        m_lastUpdatedRuleState = dto::RuleState {};
        m_lastUpdateRuleStateUserId = 0;
        m_lastDeletedRuleStateId = 0;
        m_lastDeleteRuleStateUserId = 0;
    }

private:
    RuleStatesPage m_getRuleStatesResult;
    std::optional<dto::RuleState> m_getRuleStateResult;
    std::optional<dto::RuleState> m_createRuleStateResult;
    std::optional<dto::RuleState> m_updateRuleStateResult;
    bool m_deleteRuleStateResult = false;

    int m_getRuleStatesCallCount = 0;
    int m_getRuleStateCallCount = 0;
    int m_createRuleStateCallCount = 0;
    int m_updateRuleStateCallCount = 0;
    int m_deleteRuleStateCallCount = 0;

    int m_lastGetRuleStatesPage = 0;
    int m_lastGetRuleStatesPageSize = 0;
    std::optional<int64_t> m_lastGetRuleStatesRuleId;
    std::optional<int64_t> m_lastGetRuleStatesStateId;
    int64_t m_lastGetRuleStateId = 0;
    dto::RuleState m_lastCreatedRuleState;
    int64_t m_lastCreateRuleStateUserId = 0;
    dto::RuleState m_lastUpdatedRuleState;
    int64_t m_lastUpdateRuleStateUserId = 0;
    int64_t m_lastDeletedRuleStateId = 0;
    int64_t m_lastDeleteRuleStateUserId = 0;
};

} // namespace server::tests
