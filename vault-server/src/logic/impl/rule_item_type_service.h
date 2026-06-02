#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/irule_item_type_service.h"

#include "repo/item_type_repository.h"
#include "repo/rule_item_type_repository.h"
#include "repo/rule_repository.h"

namespace server::services
{

class RuleItemTypeService final : public IRuleItemTypeService
{
public:
    RuleItemTypeService(
        std::shared_ptr<repositories::IRuleItemTypeRepository> ritRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    RuleItemTypesPage getRuleItemTypes(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) override;

    std::optional<dto::RuleItemType> getRuleItemType(int64_t id) override;

    std::optional<dto::RuleItemType> createRuleItemType(
        const dto::RuleItemType& rit,
        int64_t userId
    ) override;

    std::optional<dto::RuleItemType> updateRuleItemType(
        const dto::RuleItemType& rit,
        int64_t userId
    ) override;

    bool deleteRuleItemType(
        int64_t id,
        int64_t userId
    ) override;

private:
    void invalidateUsersByRuleId(int64_t ruleId);

private:
    std::shared_ptr<repositories::IRuleItemTypeRepository> m_ritRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IItemTypeRepository> m_itemTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
