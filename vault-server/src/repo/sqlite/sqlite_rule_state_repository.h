#pragma once

#include <memory>

#include "../rule_state_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRuleStateRepository final : public IRuleStateRepository
{
public:
    explicit SqliteRuleStateRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRuleStateRepository() override = default;

    std::pair<std::vector<dto::RuleState>, int64_t> findAll(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt
    ) override;
    std::optional<dto::RuleState> findById(int64_t id) override;
    std::vector<dto::RuleState> findByRuleId(int64_t ruleId) override;
    std::vector<dto::RuleState> findByStateId(int64_t stateId) override;
    bool exists(int64_t ruleId, int64_t stateId) override;
    int64_t create(const dto::RuleState& ruleState) override;
    bool update(const dto::RuleState& ruleState) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
