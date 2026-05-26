#pragma once

#include <memory>

#include "logic/irule_state_service.h"

#include "repo/rule_repository.h"
#include "repo/rule_state_repository.h"
#include "repo/state_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для управления правами на состояния.
 */
class RuleStateService final : public IRuleStateService
{
public:
    /**
     * @param ruleStateRepo Репозиторий прав на состояния
     * @param ruleRepo      Репозиторий правил
     * @param stateRepo     Репозиторий состояний
     */
    RuleStateService(
        std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IStateRepository> stateRepo
    );

    RuleStatesPage getRuleStates(
        int page, int pageSize,
        std::optional<int64_t> ruleId,
        std::optional<int64_t> stateId
    ) override;

    std::optional<dto::RuleState> getRuleState(int64_t id) override;
    std::optional<dto::RuleState> createRuleState(const dto::RuleState& ruleState) override;
    std::optional<dto::RuleState> updateRuleState(const dto::RuleState& ruleState) override;
    bool deleteRuleState(int64_t id) override;

private:
    std::shared_ptr<repositories::IRuleStateRepository> m_ruleStateRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IStateRepository> m_stateRepo;
};

} // namespace server::services
