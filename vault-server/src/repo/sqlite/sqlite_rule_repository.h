#pragma once

#include <memory>

#include "../rule_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server::repositories
{

class SqliteRuleRepository final : public IRuleRepository
{
public:
    explicit SqliteRuleRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteRuleRepository() override = default;

    std::pair<std::vector<dto::Rule>, int64_t> findAll(
        int page, int pageSize, std::optional<int64_t> roleId = std::nullopt
    ) override;
    std::optional<dto::Rule> findById(int64_t id) override;
    std::optional<dto::Rule> findByRoleId(int64_t roleId) override;
    int64_t create(const dto::Rule& rule) override;
    bool update(const dto::Rule& rule) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
