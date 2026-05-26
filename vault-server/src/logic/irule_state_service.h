#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_state.h"

namespace server::services
{

struct RuleStatesPage
{
    std::vector<dto::RuleState> items;
    int64_t totalCount = 0;
};

class IRuleStateService
{
public:
    virtual ~IRuleStateService() = default;

    virtual RuleStatesPage getRuleStates(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RuleState> getRuleState(int64_t id) = 0;
    virtual std::optional<dto::RuleState> createRuleState(const dto::RuleState& rs) = 0;
    virtual std::optional<dto::RuleState> updateRuleState(const dto::RuleState& rs) = 0;
    virtual bool deleteRuleState(int64_t id) = 0;
};

} // namespace server::services
