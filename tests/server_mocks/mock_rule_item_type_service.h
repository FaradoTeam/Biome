#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/rule_item_type.h"

#include "logic/irule_item_type_service.h"

namespace server::tests
{

class MockRuleItemTypeService : public services::IRuleItemTypeService
{
public:
    using RuleItemTypesPage = services::RuleItemTypesPage;

    void setGetRuleItemTypesResult(const RuleItemTypesPage& result)
    {
        m_getRuleItemTypesResult = result;
    }

    void setGetRuleItemTypeResult(std::optional<dto::RuleItemType> rit)
    {
        m_getRuleItemTypeResult = std::move(rit);
    }

    void setCreateRuleItemTypeResult(std::optional<dto::RuleItemType> rit)
    {
        m_createRuleItemTypeResult = std::move(rit);
    }

    void setUpdateRuleItemTypeResult(std::optional<dto::RuleItemType> rit)
    {
        m_updateRuleItemTypeResult = std::move(rit);
    }

    void setDeleteRuleItemTypeResult(bool result)
    {
        m_deleteRuleItemTypeResult = result;
    }

    // Реализация интерфейса
    RuleItemTypesPage getRuleItemTypes(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) override
    {
        m_lastGetRuleItemTypesPage = page;
        m_lastGetRuleItemTypesPageSize = pageSize;
        m_lastGetRuleItemTypesRuleId = ruleId;
        m_lastGetRuleItemTypesItemTypeId = itemTypeId;
        ++m_getRuleItemTypesCallCount;
        return m_getRuleItemTypesResult;
    }

    std::optional<dto::RuleItemType> getRuleItemType(int64_t id) override
    {
        m_lastGetRuleItemTypeId = id;
        ++m_getRuleItemTypeCallCount;
        return m_getRuleItemTypeResult;
    }

    std::optional<dto::RuleItemType> createRuleItemType(
        const dto::RuleItemType& rit,
        int64_t userId
    ) override
    {
        m_lastCreatedRuleItemType = rit;
        m_lastCreateRuleItemTypeUserId = userId;
        ++m_createRuleItemTypeCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createRuleItemTypeResult;
    }

    std::optional<dto::RuleItemType> updateRuleItemType(
        const dto::RuleItemType& rit,
        int64_t userId
    ) override
    {
        m_lastUpdatedRuleItemType = rit;
        m_lastUpdateRuleItemTypeUserId = userId;
        ++m_updateRuleItemTypeCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateRuleItemTypeResult;
    }

    bool deleteRuleItemType(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedRuleItemTypeId = id;
        m_lastDeleteRuleItemTypeUserId = userId;
        ++m_deleteRuleItemTypeCallCount;

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteRuleItemTypeResult;
    }

    // Методы для проверки
    int getGetRuleItemTypesCallCount() const { return m_getRuleItemTypesCallCount; }
    int getGetRuleItemTypeCallCount() const { return m_getRuleItemTypeCallCount; }
    int getCreateRuleItemTypeCallCount() const { return m_createRuleItemTypeCallCount; }
    int getUpdateRuleItemTypeCallCount() const { return m_updateRuleItemTypeCallCount; }
    int getDeleteRuleItemTypeCallCount() const { return m_deleteRuleItemTypeCallCount; }

    int getLastGetRuleItemTypesPage() const { return m_lastGetRuleItemTypesPage; }
    int getLastGetRuleItemTypesPageSize() const { return m_lastGetRuleItemTypesPageSize; }
    std::optional<int64_t> getLastGetRuleItemTypesRuleId() const { return m_lastGetRuleItemTypesRuleId; }
    std::optional<int64_t> getLastGetRuleItemTypesItemTypeId() const { return m_lastGetRuleItemTypesItemTypeId; }
    int64_t getLastGetRuleItemTypeId() const { return m_lastGetRuleItemTypeId; }
    const dto::RuleItemType& getLastCreatedRuleItemType() const { return m_lastCreatedRuleItemType; }
    int64_t getLastCreateRuleItemTypeUserId() const { return m_lastCreateRuleItemTypeUserId; }
    const dto::RuleItemType& getLastUpdatedRuleItemType() const { return m_lastUpdatedRuleItemType; }
    int64_t getLastUpdateRuleItemTypeUserId() const { return m_lastUpdateRuleItemTypeUserId; }
    int64_t getLastDeletedRuleItemTypeId() const { return m_lastDeletedRuleItemTypeId; }
    int64_t getLastDeleteRuleItemTypeUserId() const { return m_lastDeleteRuleItemTypeUserId; }

    void reset()
    {
        m_getRuleItemTypesCallCount = 0;
        m_getRuleItemTypeCallCount = 0;
        m_createRuleItemTypeCallCount = 0;
        m_updateRuleItemTypeCallCount = 0;
        m_deleteRuleItemTypeCallCount = 0;
        m_lastGetRuleItemTypesPage = 0;
        m_lastGetRuleItemTypesPageSize = 0;
        m_lastGetRuleItemTypesRuleId.reset();
        m_lastGetRuleItemTypesItemTypeId.reset();
        m_lastGetRuleItemTypeId = 0;
        m_lastCreatedRuleItemType = dto::RuleItemType {};
        m_lastCreateRuleItemTypeUserId = 0;
        m_lastUpdatedRuleItemType = dto::RuleItemType {};
        m_lastUpdateRuleItemTypeUserId = 0;
        m_lastDeletedRuleItemTypeId = 0;
        m_lastDeleteRuleItemTypeUserId = 0;
    }

private:
    RuleItemTypesPage m_getRuleItemTypesResult;
    std::optional<dto::RuleItemType> m_getRuleItemTypeResult;
    std::optional<dto::RuleItemType> m_createRuleItemTypeResult;
    std::optional<dto::RuleItemType> m_updateRuleItemTypeResult;
    bool m_deleteRuleItemTypeResult = false;

    int m_getRuleItemTypesCallCount = 0;
    int m_getRuleItemTypeCallCount = 0;
    int m_createRuleItemTypeCallCount = 0;
    int m_updateRuleItemTypeCallCount = 0;
    int m_deleteRuleItemTypeCallCount = 0;

    int m_lastGetRuleItemTypesPage = 0;
    int m_lastGetRuleItemTypesPageSize = 0;
    std::optional<int64_t> m_lastGetRuleItemTypesRuleId;
    std::optional<int64_t> m_lastGetRuleItemTypesItemTypeId;
    int64_t m_lastGetRuleItemTypeId = 0;
    dto::RuleItemType m_lastCreatedRuleItemType;
    int64_t m_lastCreateRuleItemTypeUserId = 0;
    dto::RuleItemType m_lastUpdatedRuleItemType;
    int64_t m_lastUpdateRuleItemTypeUserId = 0;
    int64_t m_lastDeletedRuleItemTypeId = 0;
    int64_t m_lastDeleteRuleItemTypeUserId = 0;
};

} // namespace server::tests
