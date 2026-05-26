#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule_project.h"

namespace server::services
{

struct RuleProjectsPage
{
    std::vector<dto::RuleProject> items;
    int64_t totalCount = 0;
};

class IRuleProjectService
{
public:
    virtual ~IRuleProjectService() = default;

    virtual RuleProjectsPage getRuleProjects(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) = 0;

    virtual std::optional<dto::RuleProject> getRuleProject(int64_t id) = 0;
    virtual std::optional<dto::RuleProject> createRuleProject(const dto::RuleProject& ruleProject) = 0;
    virtual std::optional<dto::RuleProject> updateRuleProject(const dto::RuleProject& ruleProject) = 0;
    virtual bool deleteRuleProject(int64_t id) = 0;
};

} // namespace server::services
