#pragma once

#include <memory>

#include "../plan_item_repository.h"

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
 * @brief SQLite-реализация репозитория элементов планов.
 */
class SqlitePlanItemRepository final : public IPlanItemRepository
{
public:
    explicit SqlitePlanItemRepository(std::shared_ptr<db::IDatabase> database);
    ~SqlitePlanItemRepository() override = default;

    SqlitePlanItemRepository(const SqlitePlanItemRepository&) = delete;
    SqlitePlanItemRepository& operator=(const SqlitePlanItemRepository&) = delete;

    std::pair<std::vector<dto::PlanItem>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> planId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt
    ) override;

    std::optional<dto::PlanItem> findById(int64_t id) override;
    std::optional<dto::PlanItem> findByPlanAndItem(
        int64_t planId,
        int64_t itemId
    ) override;
    std::vector<dto::PlanItem> findByPlanId(int64_t planId) override;
    std::vector<dto::PlanItem> findByUserId(int64_t userId) override;
    std::vector<dto::PlanItem> findByDateRange(
        const common::DateTime& dateFrom,
        const common::DateTime& dateTo
    ) override;
    int64_t create(const dto::PlanItem& planItem) override;
    bool update(const dto::PlanItem& planItem) override;
    bool remove(int64_t id) override;
    int64_t removeByPlanId(int64_t planId) override;
    bool exists(int64_t id) override;
    bool existsByPlanAndItem(int64_t planId, int64_t itemId) override;
    int64_t copyFromPlan(int64_t sourcePlanId, int64_t targetPlanId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::PlanItem mapRowToPlanItem(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
