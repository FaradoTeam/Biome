#pragma once

#include <memory>

#include "../rule_item_type_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRuleItemTypeRepository final : public IRuleItemTypeRepository
{
public:
    explicit SqliteRuleItemTypeRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRuleItemTypeRepository() override = default;

    std::pair<std::vector<dto::RuleItemType>, int64_t> findAll(
        int page, int pageSize,
        std::optional<int64_t> ruleId = std::nullopt,
        std::optional<int64_t> itemTypeId = std::nullopt
    ) override;
    std::optional<dto::RuleItemType> findById(int64_t id) override;
    std::vector<dto::RuleItemType> findByRuleId(int64_t ruleId) override;
    std::vector<dto::RuleItemType> findByItemTypeId(int64_t itemTypeId) override;
    bool exists(int64_t ruleId, int64_t itemTypeId) override;
    int64_t create(const dto::RuleItemType& ruleItemType) override;
    bool update(const dto::RuleItemType& ruleItemType) override;
    bool remove(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
