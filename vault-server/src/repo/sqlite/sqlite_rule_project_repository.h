#pragma once

#include <memory>

#include "../rule_project_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRuleProjectRepository final : public IRuleProjectRepository
{
public:
    explicit SqliteRuleProjectRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRuleProjectRepository() override = default;

    std::pair<std::vector<dto::RuleProject>, int64_t> findAll(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> projectId = std::nullopt
    ) override;
    std::optional<dto::RuleProject> findById(int64_t id) override;
    std::vector<dto::RuleProject> findByRuleId(int64_t ruleId) override;
    std::vector<dto::RuleProject> findByProjectId(int64_t projectId) override;
    bool exists(int64_t ruleId, int64_t projectId) override;
    int64_t create(const dto::RuleProject& ruleProject) override;
    bool update(const dto::RuleProject& ruleProject) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
