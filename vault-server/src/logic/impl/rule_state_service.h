#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/irule_state_service.h"

#include "repo/rule_repository.h"
#include "repo/rule_state_repository.h"
#include "repo/state_repository.h"

namespace server::services
{

class RuleStateService final : public IRuleStateService
{
public:
    RuleStateService(
        std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IStateRepository> stateRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    RuleStatesPage getRuleStates(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override;

    std::optional<dto::RuleState> getRuleState(int64_t id) override;
    std::optional<dto::RuleState> createRuleState(const dto::RuleState& ruleState) override;
    std::optional<dto::RuleState> updateRuleState(const dto::RuleState& ruleState) override;
    bool deleteRuleState(int64_t id) override;

private:
    void invalidateUsersByRuleId(int64_t ruleId);

private:
    std::shared_ptr<repositories::IRuleStateRepository> m_ruleStateRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IStateRepository> m_stateRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
