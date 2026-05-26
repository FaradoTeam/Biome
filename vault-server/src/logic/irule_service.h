#pragma once

#include <optional>
#include <vector>

#include "common/dto/rule.h"

namespace server::services
{

struct RulesPage
{
    std::vector<dto::Rule> rules;
    int64_t totalCount = 0;
};

class IRuleService
{
public:
    virtual ~IRuleService() = default;

    virtual RulesPage getRules(int page, int pageSize, std::optional<int64_t> roleId = std::nullopt) = 0;
    virtual std::optional<dto::Rule> getRule(int64_t id) = 0;
    virtual std::optional<dto::Rule> getRuleByRoleId(int64_t roleId) = 0;
    virtual std::optional<dto::Rule> createRule(const dto::Rule& rule) = 0;
    virtual std::optional<dto::Rule> updateRule(const dto::Rule& rule) = 0;
    virtual bool deleteRule(int64_t id) = 0;
};

} // namespace server::services
