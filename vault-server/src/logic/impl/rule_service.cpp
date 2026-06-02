#include "common/log/log.h"

#include "rule_service.h"

namespace server::services
{

RuleService::RuleService(
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_ruleRepo(std::move(ruleRepo))
    , m_roleRepo(std::move(roleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_ruleRepo || !m_roleRepo)
    {
        throw std::runtime_error("RuleService: репозитории не инициализированы");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("RuleService: сервис авторизации не инициализирован");
    }
}

RulesPage RuleService::getRules(
    int page, int pageSize,
    std::optional<int64_t> roleId
)
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

std::optional<dto::Rule> RuleService::createRule(const dto::Rule& rule, int64_t userId)
{
    // Только супер-админ может создавать правила
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createRule: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!rule.roleId.has_value())
    {
        LOG_WARN << "createRule: roleId обязателен";
        return std::nullopt;
    }

    // Проверяем существование роли
    if (!m_roleRepo->exists(*rule.roleId))
    {
        LOG_WARN << "createRule: роль не найдена, roleId=" << *rule.roleId;
        return std::nullopt;
    }

    // У одной роли не может быть нескольких правил
    if (m_ruleRepo->findByRoleId(*rule.roleId).has_value())
    {
        LOG_WARN << "createRule: правило для roleId=" << *rule.roleId << " уже существует";
        return std::nullopt;
    }

    int64_t newId = m_ruleRepo->create(rule);
    if (newId <= 0)
    {
        LOG_ERROR << "createRule: не удалось создать правило";
        return std::nullopt;
    }

    LOG_INFO
        << "Правило создано: id=" << newId
        << ", roleId=" << *rule.roleId
        << ", пользователь=" << userId;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRoleId(*rule.roleId);

    return m_ruleRepo->findById(newId);
}

std::optional<dto::Rule> RuleService::updateRule(const dto::Rule& rule, int64_t userId)
{
    // Только супер-админ может обновлять правила
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateRule: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!rule.id.has_value())
    {
        LOG_WARN << "updateRule: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_ruleRepo->findById(*rule.id);
    if (!existing)
    {
        LOG_WARN << "updateRule: правило не найдено, id=" << *rule.id;
        return std::nullopt;
    }

    const int64_t oldRoleId = *existing->roleId;
    int64_t newRoleId = oldRoleId;

    // Если меняется roleId, проверяем существование новой роли и уникальность
    if (rule.roleId.has_value() && *rule.roleId != oldRoleId)
    {
        if (!m_roleRepo->exists(*rule.roleId))
        {
            LOG_WARN << "updateRule: новая roleId не найдена, roleId=" << *rule.roleId;
            return std::nullopt;
        }
        if (m_ruleRepo->findByRoleId(*rule.roleId).has_value())
        {
            LOG_WARN << "updateRule: правило для новой roleId=" << *rule.roleId << " уже существует";
            return std::nullopt;
        }
        newRoleId = *rule.roleId;
    }

    if (!m_ruleRepo->update(rule))
    {
        LOG_ERROR << "updateRule: не удалось обновить правило id=" << *rule.id;
        return std::nullopt;
    }

    LOG_INFO << "Правило обновлено: id=" << *rule.id << ", пользователь=" << userId;

    // Инвалидируем кэш по старой и новой роли
    invalidateUsersByRoleId(oldRoleId);
    if (newRoleId != oldRoleId)
    {
        invalidateUsersByRoleId(newRoleId);
    }

    return m_ruleRepo->findById(*rule.id);
}

bool RuleService::deleteRule(int64_t id, int64_t userId)
{
    // Только супер-админ может удалять правила
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteRule: пользователь " << userId << " не имеет прав";
        return false;
    }

    auto existing = m_ruleRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRule: правило не найдено, id=" << id;
        return false;
    }

    const int64_t roleId = *existing->roleId;

    // TODO: проверить, что правило не используется в RuleProject, RuleItemType, RuleState
    if (!m_ruleRepo->remove(id))
    {
        LOG_ERROR << "deleteRule: не удалось удалить правило id=" << id;
        return false;
    }

    LOG_INFO << "Правило удалено: id=" << id << ", пользователь=" << userId;

    // Инвалидируем кэш для всех пользователей с этой ролью
    invalidateUsersByRoleId(roleId);

    return true;
}

void RuleService::invalidateUsersByRoleId(int64_t roleId)
{
    auto users = m_authzService->getUserIdsByRoleId(roleId);
    LOG_DEBUG
        << "Инвалидация кэша для " << users.size()
        << " пользователей с ролью " << roleId;

    for (int64_t userId : users)
    {
        m_authzService->invalidateCache(userId);
        LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
    }
}

} // namespace server::services
