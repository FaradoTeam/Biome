#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_state.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для прав на состояния (RuleState).
 */
class IRuleStateRepository
{
public:
    virtual ~IRuleStateRepository() = default;

    virtual std::pair<std::vector<dto::RuleState>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RuleState> findById(int64_t id) = 0;
    virtual std::vector<dto::RuleState> findByRuleId(int64_t ruleId) = 0;
    virtual std::vector<dto::RuleState> findByStateId(int64_t stateId) = 0;
    virtual bool exists(int64_t ruleId, int64_t stateId) = 0;
    virtual int64_t create(const dto::RuleState& ruleState) = 0;
    virtual bool update(const dto::RuleState& ruleState) = 0;
    virtual bool remove(int64_t id) = 0;
};

} // namespace server::repositories
