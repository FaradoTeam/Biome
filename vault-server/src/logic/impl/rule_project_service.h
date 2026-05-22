#pragma once

#include <memory>

#include "logic/irule_project_service.h"

#include "repo/project_repository.h"
#include "repo/rule_project_repository.h"
#include "repo/rule_repository.h"

namespace server::services
{

class RuleProjectService final : public IRuleProjectService
{
public:
    explicit RuleProjectService(
        std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IProjectRepository> projectRepo
    );

    RuleProjectsPage getRuleProjects(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) override;

    std::optional<dto::RuleProject> getRuleProject(int64_t id) override;
    std::optional<dto::RuleProject> createRuleProject(const dto::RuleProject& ruleProject) override;
    std::optional<dto::RuleProject> updateRuleProject(const dto::RuleProject& ruleProject) override;
    bool deleteRuleProject(int64_t id) override;

private:
    std::shared_ptr<repositories::IRuleProjectRepository> m_ruleProjectRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IProjectRepository> m_projectRepo;
};

} // namespace server::services
