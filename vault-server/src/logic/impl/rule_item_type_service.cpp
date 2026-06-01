#include "common/log/log.h"

#include "rule_item_type_service.h"

namespace server::services
{

RuleItemTypeService::RuleItemTypeService(
    std::shared_ptr<repositories::IRuleItemTypeRepository> ritRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_ritRepo(std::move(ritRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_itemTypeRepo(std::move(itemTypeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_ritRepo || !m_ruleRepo || !m_itemTypeRepo)
    {
        throw std::runtime_error("RuleItemTypeService: repositories are null");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("RuleItemTypeService: authorizationService is null");
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
    return { std::move(items), total };
}

std::optional<dto::RuleItemType> RuleItemTypeService::getRuleItemType(int64_t id)
{
    return m_ritRepo->findById(id);
}

std::optional<dto::RuleItemType> RuleItemTypeService::createRuleItemType(const dto::RuleItemType& rit)
{
    // Проверка обязательных полей
    if (!rit.ruleId.has_value() || !rit.itemTypeId.has_value())
    {
        LOG_WARN << "createRuleItemType: ruleId and itemTypeId are required";
        return std::nullopt;
    }

    // Проверка существования правила
    if (!m_ruleRepo->exists(*rit.ruleId))
    {
        LOG_WARN << "createRuleItemType: rule with id=" << *rit.ruleId << " not found";
        return std::nullopt;
    }

    // Проверка существования типа элемента
    if (!m_itemTypeRepo->exists(*rit.itemTypeId))
    {
        LOG_WARN << "createRuleItemType: itemType with id=" << *rit.itemTypeId << " not found";
        return std::nullopt;
    }

    // Проверка уникальности пары (ruleId, itemTypeId)
    if (m_ritRepo->exists(*rit.ruleId, *rit.itemTypeId))
    {
        LOG_WARN << "createRuleItemType: pair (ruleId,itemTypeId) already exists";
        return std::nullopt;
    }

    int64_t newId = m_ritRepo->create(rit);
    if (newId <= 0)
    {
        LOG_ERROR << "createRuleItemType: failed to create RuleItemType";
        return std::nullopt;
    }

    LOG_INFO << "RuleItemType created: id=" << newId
             << ", ruleId=" << *rit.ruleId
             << ", itemTypeId=" << *rit.itemTypeId;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRuleId(*rit.ruleId);

    return m_ritRepo->findById(newId);
}

std::optional<dto::RuleItemType> RuleItemTypeService::updateRuleItemType(const dto::RuleItemType& rit)
{
    if (!rit.id.has_value())
    {
        LOG_WARN << "updateRuleItemType: missing id";
        return std::nullopt;
    }

    auto existing = m_ritRepo->findById(*rit.id);
    if (!existing)
    {
        LOG_WARN << "updateRuleItemType: RuleItemType with id=" << *rit.id << " not found";
        return std::nullopt;
    }

    // Проверка уникальности при изменении ruleId или itemTypeId
    bool needUniquenessCheck = false;
    int64_t oldRuleId = *existing->ruleId;
    int64_t newRuleId = oldRuleId;
    int64_t newItemTypeId = *existing->itemTypeId;

    if (rit.ruleId.has_value() && *rit.ruleId != oldRuleId)
    {
        if (!m_ruleRepo->exists(*rit.ruleId))
        {
            LOG_WARN << "updateRuleItemType: new ruleId=" << *rit.ruleId << " not found";
            return std::nullopt;
        }
        newRuleId = *rit.ruleId;
        needUniquenessCheck = true;
    }

    if (rit.itemTypeId.has_value() && *rit.itemTypeId != newItemTypeId)
    {
        if (!m_itemTypeRepo->exists(*rit.itemTypeId))
        {
            LOG_WARN << "updateRuleItemType: new itemTypeId=" << *rit.itemTypeId << " not found";
            return std::nullopt;
        }
        newItemTypeId = *rit.itemTypeId;
        needUniquenessCheck = true;
    }

    if (needUniquenessCheck && m_ritRepo->exists(newRuleId, newItemTypeId))
    {
        LOG_WARN << "updateRuleItemType: pair (ruleId,itemTypeId) already exists";
        return std::nullopt;
    }

    if (!m_ritRepo->update(rit))
    {
        LOG_ERROR << "updateRuleItemType: failed to update RuleItemType id=" << *rit.id;
        return std::nullopt;
    }

    LOG_INFO << "RuleItemType updated: id=" << *rit.id;

    // Инвалидируем кэш по старому и новому ruleId
    invalidateUsersByRuleId(oldRuleId);
    if (rit.ruleId.has_value() && *rit.ruleId != oldRuleId)
    {
        invalidateUsersByRuleId(*rit.ruleId);
    }

    return m_ritRepo->findById(*rit.id);
}

bool RuleItemTypeService::deleteRuleItemType(int64_t id)
{
    auto existing = m_ritRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRuleItemType: RuleItemType with id=" << id << " not found";
        return false;
    }

    int64_t ruleId = *existing->ruleId;

    if (!m_ritRepo->remove(id))
    {
        LOG_ERROR << "deleteRuleItemType: failed to delete RuleItemType id=" << id;
        return false;
    }

    LOG_INFO << "RuleItemType deleted: id=" << id;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRuleId(ruleId);

    return true;
}

void RuleItemTypeService::invalidateUsersByRuleId(int64_t ruleId)
{
    auto rule = m_ruleRepo->findById(ruleId);
    if (!rule || !rule->roleId.has_value())
    {
        LOG_DEBUG << "invalidateUsersByRuleId: rule " << ruleId << " has no roleId";
        return;
    }

    auto users = m_authzService->getUserIdsByRoleId(*rule->roleId);
    LOG_DEBUG << "Инвалидация кэша для " << users.size() << " пользователей с ролью " << *rule->roleId;

    for (int64_t userId : users)
    {
        m_authzService->invalidateCache(userId);
        LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
    }
}

} // namespace server::services
