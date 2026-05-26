#include "common/log/log.h"

#include "rule_service.h"

namespace server::services
{

RuleService::RuleService(std::shared_ptr<repositories::IRuleRepository> ruleRepo, std::shared_ptr<repositories::IRoleRepository> roleRepo)
    : m_ruleRepo(std::move(ruleRepo))
    , m_roleRepo(std::move(roleRepo))
{
    if (!m_ruleRepo || !m_roleRepo)
    {
        throw std::runtime_error("RuleService: repositories are null");
    }
}

RulesPage RuleService::getRules(int page, int pageSize, std::optional<int64_t> roleId)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [rules, total] = m_ruleRepo->findAll(page, pageSize, roleId);
    return { rules, total };
}

std::optional<dto::Rule> RuleService::getRule(int64_t id)
{
    return m_ruleRepo->findById(id);
}

std::optional<dto::Rule> RuleService::getRuleByRoleId(int64_t roleId)
{
    return m_ruleRepo->findByRoleId(roleId);
}

std::optional<dto::Rule> RuleService::createRule(const dto::Rule& rule)
{
    if (!rule.roleId.has_value())
    {
        LOG_WARN << "createRule: roleId is required";
        return std::nullopt;
    }

    // Проверяем существование роли
    if (!m_roleRepo->exists(*rule.roleId))
    {
        LOG_WARN << "createRule: role not found, roleId=" << *rule.roleId;
        return std::nullopt;
    }

    // У одной роли не может быть нескольких правил (business rule)
    if (m_ruleRepo->findByRoleId(*rule.roleId).has_value())
    {
        LOG_WARN << "createRule: rule already exists for roleId=" << *rule.roleId;
        return std::nullopt;
    }

    int64_t newId = m_ruleRepo->create(rule);
    if (newId <= 0)
    {
        LOG_ERROR << "createRule: failed to create rule";
        return std::nullopt;
    }

    LOG_INFO << "Rule created: id=" << newId << ", roleId=" << *rule.roleId;
    return m_ruleRepo->findById(newId);
}

std::optional<dto::Rule> RuleService::updateRule(const dto::Rule& rule)
{
    if (!rule.id.has_value())
    {
        LOG_WARN << "updateRule: missing id";
        return std::nullopt;
    }

    auto existing = m_ruleRepo->findById(*rule.id);
    if (!existing)
    {
        LOG_WARN << "updateRule: rule not found, id=" << *rule.id;
        return std::nullopt;
    }

    // Если меняется roleId, проверяем существование новой роли и уникальность
    if (rule.roleId.has_value() && *rule.roleId != *existing->roleId)
    {
        if (!m_roleRepo->exists(*rule.roleId))
        {
            LOG_WARN << "updateRule: new roleId not found, roleId=" << *rule.roleId;
            return std::nullopt;
        }
        if (m_ruleRepo->findByRoleId(*rule.roleId).has_value())
        {
            LOG_WARN << "updateRule: rule already exists for new roleId=" << *rule.roleId;
            return std::nullopt;
        }
    }

    if (!m_ruleRepo->update(rule))
    {
        LOG_ERROR << "updateRule: failed to update rule id=" << *rule.id;
        return std::nullopt;
    }

    LOG_INFO << "Rule updated: id=" << *rule.id;
    return m_ruleRepo->findById(*rule.id);
}

bool RuleService::deleteRule(int64_t id)
{
    // TODO: проверить, что правило не используется в RuleProject, RuleItemType, RuleState
    if (!m_ruleRepo->exists(id))
    {
        LOG_WARN << "deleteRule: rule not found, id=" << id;
        return false;
    }

    if (!m_ruleRepo->remove(id))
    {
        LOG_ERROR << "deleteRule: failed to delete rule id=" << id;
        return false;
    }

    LOG_INFO << "Rule deleted: id=" << id;
    return true;
}

} // namespace server::services
