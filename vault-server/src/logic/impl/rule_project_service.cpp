#include "common/log/log.h"

#include "rule_project_service.h"

namespace server::services
{

RuleProjectService::RuleProjectService(
    std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IProjectRepository> projectRepo
)
    : m_ruleProjectRepo(std::move(ruleProjectRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_projectRepo(std::move(projectRepo))
{
    if (!m_ruleProjectRepo || !m_ruleRepo || !m_projectRepo)
    {
        throw std::runtime_error("RuleProjectService: repositories are null");
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
        LOG_WARN << "createRuleProject: ruleId and projectId are required";
        return std::nullopt;
    }

    // Проверяем существование правила и проекта
    if (!m_ruleRepo->exists(*rp.ruleId))
    {
        LOG_WARN << "createRuleProject: rule not found, ruleId=" << *rp.ruleId;
        return std::nullopt;
    }
    if (!m_projectRepo->exists(*rp.projectId))
    {
        LOG_WARN << "createRuleProject: project not found, projectId=" << *rp.projectId;
        return std::nullopt;
    }

    // Проверяем уникальность пары
    if (m_ruleProjectRepo->exists(*rp.ruleId, *rp.projectId))
    {
        LOG_WARN << "createRuleProject: rule-project pair already exists";
        return std::nullopt;
    }

    int64_t newId = m_ruleProjectRepo->create(rp);
    if (newId <= 0)
    {
        LOG_ERROR << "createRuleProject: failed to create";
        return std::nullopt;
    }

    LOG_INFO << "RuleProject created: id=" << newId
             << ", ruleId=" << *rp.ruleId << ", projectId=" << *rp.projectId;
    return m_ruleProjectRepo->findById(newId);
}

std::optional<dto::RuleProject> RuleProjectService::updateRuleProject(const dto::RuleProject& rp)
{
    if (!rp.id.has_value())
    {
        LOG_WARN << "updateRuleProject: missing id";
        return std::nullopt;
    }

    auto existing = m_ruleProjectRepo->findById(*rp.id);
    if (!existing)
    {
        LOG_WARN << "updateRuleProject: not found, id=" << *rp.id;
        return std::nullopt;
    }

    // Если меняется ruleId или projectId, проверяем существование и уникальность
    bool needCheckUnique = false;
    if (rp.ruleId.has_value() && *rp.ruleId != *existing->ruleId)
    {
        if (!m_ruleRepo->exists(*rp.ruleId))
        {
            LOG_WARN << "updateRuleProject: new ruleId not found";
            return std::nullopt;
        }
        needCheckUnique = true;
    }
    if (rp.projectId.has_value() && *rp.projectId != *existing->projectId)
    {
        if (!m_projectRepo->exists(*rp.projectId))
        {
            LOG_WARN << "updateRuleProject: new projectId not found";
            return std::nullopt;
        }
        needCheckUnique = true;
    }
    if (needCheckUnique)
    {
        int64_t newRuleId = rp.ruleId.has_value() ? *rp.ruleId : *existing->ruleId;
        int64_t newProjectId = rp.projectId.has_value() ? *rp.projectId : *existing->projectId;
        if (m_ruleProjectRepo->exists(newRuleId, newProjectId))
        {
            LOG_WARN << "updateRuleProject: pair (ruleId,projectId) already exists";
            return std::nullopt;
        }
    }

    if (!m_ruleProjectRepo->update(rp))
    {
        LOG_ERROR << "updateRuleProject: failed to update id=" << *rp.id;
        return std::nullopt;
    }

    LOG_INFO << "RuleProject updated: id=" << *rp.id;
    return m_ruleProjectRepo->findById(*rp.id);
}

bool RuleProjectService::deleteRuleProject(int64_t id)
{
    if (!m_ruleProjectRepo->findById(id).has_value())
    {
        LOG_WARN << "deleteRuleProject: not found, id=" << id;
        return false;
    }

    if (!m_ruleProjectRepo->remove(id))
    {
        LOG_ERROR << "deleteRuleProject: failed to delete id=" << id;
        return false;
    }

    LOG_INFO << "RuleProject deleted: id=" << id;
    return true;
}

} // namespace server::services
