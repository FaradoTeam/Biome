#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/irule_service.h"

#include "repo/role_repository.h"
#include "repo/rule_repository.h"

namespace server::services
{

class RuleService final : public IRuleService
{
public:
    RuleService(
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IRoleRepository> roleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    RulesPage getRules(
        int page, int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) override;

    std::optional<dto::Rule> getRule(int64_t id) override;
    std::optional<dto::Rule> getRuleByRoleId(int64_t roleId) override;

    std::optional<dto::Rule> createRule(
        const dto::Rule& rule,
        int64_t userId
    ) override;

    std::optional<dto::Rule> updateRule(
        const dto::Rule& rule,
        int64_t userId
    ) override;

    bool deleteRule(
        int64_t id,
        int64_t userId
    ) override;

private:
    void invalidateUsersByRoleId(int64_t roleId);

private:
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IRoleRepository> m_roleRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
