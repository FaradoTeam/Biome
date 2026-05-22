#pragma once

#include <memory>

#include "logic/irule_item_type_service.h"

#include "repo/item_type_repository.h"
#include "repo/rule_item_type_repository.h"
#include "repo/rule_repository.h"

namespace server::services
{

class RuleItemTypeService final : public IRuleItemTypeService
{
public:
    explicit RuleItemTypeService(
        std::shared_ptr<repositories::IRuleItemTypeRepository> ritRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo
    );

    RuleItemTypesPage getRuleItemTypes(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) override;

    std::optional<dto::RuleItemType> getRuleItemType(int64_t id) override;
    std::optional<dto::RuleItemType> createRuleItemType(const dto::RuleItemType& rit) override;
    std::optional<dto::RuleItemType> updateRuleItemType(const dto::RuleItemType& rit) override;
    bool deleteRuleItemType(int64_t id) override;

private:
    std::shared_ptr<repositories::IRuleItemTypeRepository> m_ritRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IItemTypeRepository> m_itemTypeRepo;
};

} // namespace server::services
