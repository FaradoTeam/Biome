#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/irule_project_service.h"

#include "repo/project_repository.h"
#include "repo/rule_project_repository.h"
#include "repo/rule_repository.h"

namespace server::services
{

class RuleProjectService final : public IRuleProjectService
{
public:
    RuleProjectService(
        std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IProjectRepository> projectRepo,
        std::shared_ptr<IAuthorizationService> authzService
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
    void invalidateUsersByRuleId(int64_t ruleId);

private:
    std::shared_ptr<repositories::IRuleProjectRepository> m_ruleProjectRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IProjectRepository> m_projectRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
