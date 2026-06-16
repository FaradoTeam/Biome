#pragma once

#include <memory>

#include "../plan_repository.h"

namespace db
{
class IDatabase;
class IConnection;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite-реализация репозитория планов.
 */
class SqlitePlanRepository final : public IPlanRepository
{
public:
    explicit SqlitePlanRepository(std::shared_ptr<db::IDatabase> database);
    ~SqlitePlanRepository() override = default;

    SqlitePlanRepository(const SqlitePlanRepository&) = delete;
    SqlitePlanRepository& operator=(const SqlitePlanRepository&) = delete;

    std::pair<std::vector<dto::Plan>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<bool> isActive = std::nullopt
    ) override;

    std::optional<dto::Plan> findById(int64_t id) override;
    std::optional<dto::Plan> findActiveByPhaseId(int64_t phaseId) override;
    int64_t create(const dto::Plan& plan) override;
    bool update(const dto::Plan& plan) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    int64_t deactivateAllByPhaseId(int64_t phaseId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::Plan mapRowToPlan(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
