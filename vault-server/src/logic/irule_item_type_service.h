#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_item_type.h"

namespace server::services
{

struct RuleItemTypesPage
{
    std::vector<dto::RuleItemType> items;
    int64_t totalCount = 0;
};

class IRuleItemTypeService
{
public:
    virtual ~IRuleItemTypeService() = default;

    virtual RuleItemTypesPage getRuleItemTypes(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RuleItemType> getRuleItemType(int64_t id) = 0;
    virtual std::optional<dto::RuleItemType> createRuleItemType(const dto::RuleItemType& rit) = 0;
    virtual std::optional<dto::RuleItemType> updateRuleItemType(const dto::RuleItemType& rit) = 0;
    virtual bool deleteRuleItemType(int64_t id) = 0;
};

} // namespace server::services
