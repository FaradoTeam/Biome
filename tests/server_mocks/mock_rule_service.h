#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/rule.h"

#include "logic/irule_service.h"

namespace server::tests
{

class MockRuleService : public services::IRuleService
{
public:
    using RulesPage = services::RulesPage;

    void setGetRulesResult(const RulesPage& result)
    {
        m_getRulesResult = result;
    }

    void setGetRuleResult(std::optional<dto::Rule> rule)
    {
        m_getRuleResult = std::move(rule);
    }

    void setGetRuleByRoleIdResult(std::optional<dto::Rule> rule)
    {
        m_getRuleByRoleIdResult = std::move(rule);
    }

    void setCreateRuleResult(std::optional<dto::Rule> rule)
    {
        m_createRuleResult = std::move(rule);
    }

    void setUpdateRuleResult(std::optional<dto::Rule> rule)
    {
        m_updateRuleResult = std::move(rule);
    }

    void setDeleteRuleResult(bool result)
    {
        m_deleteRuleResult = result;
    }

    // Реализация интерфейса
    RulesPage getRules(
        int page,
        int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) override
    {
        m_lastGetRulesPage = page;
        m_lastGetRulesPageSize = pageSize;
        m_lastGetRulesRoleId = roleId;
        ++m_getRulesCallCount;
        return m_getRulesResult;
    }

    std::optional<dto::Rule> getRule(int64_t id) override
    {
        m_lastGetRuleId = id;
        ++m_getRuleCallCount;
        return m_getRuleResult;
    }

    std::optional<dto::Rule> getRuleByRoleId(int64_t roleId) override
    {
        m_lastGetRuleByRoleId = roleId;
        ++m_getRuleByRoleIdCallCount;
        return m_getRuleByRoleIdResult;
    }

    std::optional<dto::Rule> createRule(
        const dto::Rule& rule,
        int64_t userId
    ) override
    {
        m_lastCreatedRule = rule;
        m_lastCreateRuleUserId = userId;
        ++m_createRuleCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createRuleResult;
    }

    std::optional<dto::Rule> updateRule(
        const dto::Rule& rule,
        int64_t userId
    ) override
    {
        m_lastUpdatedRule = rule;
        m_lastUpdateRuleUserId = userId;
        ++m_updateRuleCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateRuleResult;
    }

    bool deleteRule(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedRuleId = id;
        m_lastDeleteRuleUserId = userId;
        ++m_deleteRuleCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteRuleResult;
    }

    // Методы для проверки вызовов
    int getGetRulesCallCount() const { return m_getRulesCallCount; }
    int getGetRuleCallCount() const { return m_getRuleCallCount; }
    int getGetRuleByRoleIdCallCount() const { return m_getRuleByRoleIdCallCount; }
    int getCreateRuleCallCount() const { return m_createRuleCallCount; }
    int getUpdateRuleCallCount() const { return m_updateRuleCallCount; }
    int getDeleteRuleCallCount() const { return m_deleteRuleCallCount; }

    int getLastGetRulesPage() const { return m_lastGetRulesPage; }
    int getLastGetRulesPageSize() const { return m_lastGetRulesPageSize; }
    std::optional<int64_t> getLastGetRulesRoleId() const { return m_lastGetRulesRoleId; }
    int64_t getLastGetRuleId() const { return m_lastGetRuleId; }
    int64_t getLastGetRuleByRoleId() const { return m_lastGetRuleByRoleId; }
    const dto::Rule& getLastCreatedRule() const { return m_lastCreatedRule; }
    int64_t getLastCreateRuleUserId() const { return m_lastCreateRuleUserId; }
    const dto::Rule& getLastUpdatedRule() const { return m_lastUpdatedRule; }
    int64_t getLastUpdateRuleUserId() const { return m_lastUpdateRuleUserId; }
    int64_t getLastDeletedRuleId() const { return m_lastDeletedRuleId; }
    int64_t getLastDeleteRuleUserId() const { return m_lastDeleteRuleUserId; }

    void reset()
    {
        m_getRulesCallCount = 0;
        m_getRuleCallCount = 0;
        m_getRuleByRoleIdCallCount = 0;
        m_createRuleCallCount = 0;
        m_updateRuleCallCount = 0;
        m_deleteRuleCallCount = 0;
        m_lastGetRulesPage = 0;
        m_lastGetRulesPageSize = 0;
        m_lastGetRulesRoleId.reset();
        m_lastGetRuleId = 0;
        m_lastGetRuleByRoleId = 0;
        m_lastCreatedRule = dto::Rule {};
        m_lastCreateRuleUserId = 0;
        m_lastUpdatedRule = dto::Rule {};
        m_lastUpdateRuleUserId = 0;
        m_lastDeletedRuleId = 0;
        m_lastDeleteRuleUserId = 0;
    }

private:
    RulesPage m_getRulesResult;
    std::optional<dto::Rule> m_getRuleResult;
    std::optional<dto::Rule> m_getRuleByRoleIdResult;
    std::optional<dto::Rule> m_createRuleResult;
    std::optional<dto::Rule> m_updateRuleResult;
    bool m_deleteRuleResult = false;

    int m_getRulesCallCount = 0;
    int m_getRuleCallCount = 0;
    int m_getRuleByRoleIdCallCount = 0;
    int m_createRuleCallCount = 0;
    int m_updateRuleCallCount = 0;
    int m_deleteRuleCallCount = 0;

    int m_lastGetRulesPage = 0;
    int m_lastGetRulesPageSize = 0;
    std::optional<int64_t> m_lastGetRulesRoleId;
    int64_t m_lastGetRuleId = 0;
    int64_t m_lastGetRuleByRoleId = 0;
    dto::Rule m_lastCreatedRule;
    int64_t m_lastCreateRuleUserId = 0;
    dto::Rule m_lastUpdatedRule;
    int64_t m_lastUpdateRuleUserId = 0;
    int64_t m_lastDeletedRuleId = 0;
    int64_t m_lastDeleteRuleUserId = 0;
};

} // namespace server::tests
