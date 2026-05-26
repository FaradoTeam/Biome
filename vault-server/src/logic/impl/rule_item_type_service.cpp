#include "common/log/log.h"

#include "rule_item_type_service.h"

namespace server::services
{

RuleItemTypeService::RuleItemTypeService(
    std::shared_ptr<repositories::IRuleItemTypeRepository> ritRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo
)
    : m_ritRepo(std::move(ritRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_itemTypeRepo(std::move(itemTypeRepo))
{
    if (!m_ritRepo || !m_ruleRepo || !m_itemTypeRepo)
    {
        throw std::runtime_error("RuleItemTypeService: repositories are null");
    }
}

RuleItemTypesPage RuleItemTypeService::getRuleItemTypes(
    int page, int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> itemTypeId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_ritRepo->findAll(page, pageSize, ruleId, itemTypeId);
    return { items, total };
}

std::optional<dto::RuleItemType> RuleItemTypeService::getRuleItemType(int64_t id)
{
    return m_ritRepo->findById(id);
}

std::optional<dto::RuleItemType> RuleItemTypeService::createRuleItemType(const dto::RuleItemType& rit)
{
    if (!rit.ruleId.has_value() || !rit.itemTypeId.has_value())
    {
        LOG_WARN << "createRuleItemType: ruleId and itemTypeId are required";
        return std::nullopt;
    }

    if (!m_ruleRepo->exists(*rit.ruleId))
    {
        LOG_WARN << "createRuleItemType: rule not found";
        return std::nullopt;
    }
    if (!m_itemTypeRepo->exists(*rit.itemTypeId))
    {
        LOG_WARN << "createRuleItemType: itemType not found";
        return std::nullopt;
    }
    if (m_ritRepo->exists(*rit.ruleId, *rit.itemTypeId))
    {
        LOG_WARN << "createRuleItemType: pair already exists";
        return std::nullopt;
    }

    int64_t newId = m_ritRepo->create(rit);
    if (newId <= 0)
        return std::nullopt;

    LOG_INFO << "RuleItemType created: id=" << newId;
    return m_ritRepo->findById(newId);
}

std::optional<dto::RuleItemType> RuleItemTypeService::updateRuleItemType(const dto::RuleItemType& rit)
{
    if (!rit.id.has_value())
        return std::nullopt;

    auto existing = m_ritRepo->findById(*rit.id);
    if (!existing)
        return std::nullopt;

    // Аналогично RuleProjectService, проверяем уникальность при изменении ruleId/itemTypeId
    bool needCheck = false;
    if (rit.ruleId.has_value() && *rit.ruleId != *existing->ruleId)
    {
        if (!m_ruleRepo->exists(*rit.ruleId))
            return std::nullopt;
        needCheck = true;
    }
    if (rit.itemTypeId.has_value() && *rit.itemTypeId != *existing->itemTypeId)
    {
        if (!m_itemTypeRepo->exists(*rit.itemTypeId))
            return std::nullopt;
        needCheck = true;
    }
    if (needCheck)
    {
        int64_t newRuleId = rit.ruleId.has_value() ? *rit.ruleId : *existing->ruleId;
        int64_t newItemTypeId = rit.itemTypeId.has_value() ? *rit.itemTypeId : *existing->itemTypeId;
        if (m_ritRepo->exists(newRuleId, newItemTypeId))
        {
            LOG_WARN << "updateRuleItemType: pair already exists";
            return std::nullopt;
        }
    }

    if (!m_ritRepo->update(rit))
        return std::nullopt;
    return m_ritRepo->findById(*rit.id);
}

bool RuleItemTypeService::deleteRuleItemType(int64_t id)
{
    if (!m_ritRepo->findById(id).has_value())
        return false;
    if (!m_ritRepo->remove(id))
        return false;
    LOG_INFO << "RuleItemType deleted: id=" << id;
    return true;
}

} // namespace server::services
