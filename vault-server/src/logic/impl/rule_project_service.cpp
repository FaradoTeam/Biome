#include "common/log/log.h"

#include "rule_project_service.h"

namespace server::services
{

RuleProjectService::RuleProjectService(
    std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IProjectRepository> projectRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_ruleProjectRepo(std::move(ruleProjectRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_projectRepo(std::move(projectRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_ruleProjectRepo || !m_ruleRepo || !m_projectRepo)
    {
        throw std::runtime_error("RuleProjectService: репозитории не инициализированы");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

RuleProjectsPage RuleProjectService::getRuleProjects(
    int page, int pageSize,
    std::optional<int64_t> ruleId,
    std::optional<int64_t> projectId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_ruleProjectRepo->findAll(page, pageSize, ruleId, projectId);
    return { items, total };
}

std::optional<dto::RuleProject> RuleProjectService::getRuleProject(int64_t id)
{
    return m_ruleProjectRepo->findById(id);
}

std::optional<dto::RuleProject> RuleProjectService::createRuleProject(const dto::RuleProject& rp)
{
    if (!rp.ruleId.has_value() || !rp.projectId.has_value())
    {
        LOG_WARN << "createRuleProject: обязательны ruleId и projectId";
        return std::nullopt;
    }

    // Проверяем существование правила и проекта
    if (!m_ruleRepo->exists(*rp.ruleId))
    {
        LOG_WARN << "createRuleProject: правило не найдено, ruleId=" << *rp.ruleId;
        return std::nullopt;
    }
    if (!m_projectRepo->exists(*rp.projectId))
    {
        LOG_WARN << "createRuleProject: проект не найден, projectId=" << *rp.projectId;
        return std::nullopt;
    }

    // Проверяем уникальность пары
    if (m_ruleProjectRepo->exists(*rp.ruleId, *rp.projectId))
    {
        LOG_WARN << "createRuleProject: пара rule-project уже существует";
        return std::nullopt;
    }

    int64_t newId = m_ruleProjectRepo->create(rp);
    if (newId <= 0)
    {
        LOG_ERROR << "createRuleProject: не удалось создать запись";
        return std::nullopt;
    }

    LOG_INFO
        << "RuleProject создан: id=" << newId
        << ", ruleId=" << *rp.ruleId << ", projectId=" << *rp.projectId;

    invalidateUsersByRuleId(*rp.ruleId);

    return m_ruleProjectRepo->findById(newId);
}

std::optional<dto::RuleProject> RuleProjectService::updateRuleProject(const dto::RuleProject& rp)
{
    if (!rp.id.has_value())
    {
        LOG_WARN << "updateRuleProject: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_ruleProjectRepo->findById(*rp.id);
    if (!existing)
    {
        LOG_WARN << "updateRuleProject: запись не найдена, id=" << *rp.id;
        return std::nullopt;
    }

    bool needCheckUnique = false;
    int64_t oldRuleId = *existing->ruleId;

    if (rp.ruleId.has_value() && *rp.ruleId != oldRuleId)
    {
        if (!m_ruleRepo->exists(*rp.ruleId))
        {
            LOG_WARN << "updateRuleProject: новая ruleId не найдена";
            return std::nullopt;
        }
        needCheckUnique = true;
    }
    if (rp.projectId.has_value() && *rp.projectId != *existing->projectId)
    {
        if (!m_projectRepo->exists(*rp.projectId))
        {
            LOG_WARN << "updateRuleProject: новый projectId не найден";
            return std::nullopt;
        }
        needCheckUnique = true;
    }
    if (needCheckUnique)
    {
        int64_t newRuleId = rp.ruleId.has_value() ? *rp.ruleId : oldRuleId;
        int64_t newProjectId = rp.projectId.has_value() ? *rp.projectId : *existing->projectId;
        if (m_ruleProjectRepo->exists(newRuleId, newProjectId))
        {
            LOG_WARN << "updateRuleProject: пара (ruleId,projectId) уже существует";
            return std::nullopt;
        }
    }

    if (!m_ruleProjectRepo->update(rp))
    {
        LOG_ERROR << "updateRuleProject: не удалось обновить id=" << *rp.id;
        return std::nullopt;
    }

    LOG_INFO << "RuleProject обновлен: id=" << *rp.id;

    // Инвалидируем по старому и новому ruleId
    invalidateUsersByRuleId(oldRuleId);
    if (rp.ruleId.has_value() && *rp.ruleId != oldRuleId)
    {
        invalidateUsersByRuleId(*rp.ruleId);
    }

    return m_ruleProjectRepo->findById(*rp.id);
}

bool RuleProjectService::deleteRuleProject(int64_t id)
{
    auto existing = m_ruleProjectRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteRuleProject: запись не найдена, id=" << id;
        return false;
    }

    int64_t ruleId = *existing->ruleId;

    if (!m_ruleProjectRepo->remove(id))
    {
        LOG_ERROR << "deleteRuleProject: не удалось удалить id=" << id;
        return false;
    }

    LOG_INFO << "RuleProject удален: id=" << id;

    invalidateUsersByRuleId(ruleId);

    return true;
}

void RuleProjectService::invalidateUsersByRuleId(int64_t ruleId)
{
    auto rule = m_ruleRepo->findById(ruleId);
    if (!rule || !rule->roleId.has_value())
    {
        return;
    }

    auto users = m_authzService->getUserIdsByRoleId(*rule->roleId);
    for (int64_t userId : users)
    {
        m_authzService->invalidateCache(userId);
        LOG_DEBUG << "Инвалидирован кэш для пользователя " << userId;
    }
}

} // namespace server::services
