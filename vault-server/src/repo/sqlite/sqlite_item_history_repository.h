#pragma once

#include <memory>

#include "../item_history_repository.h"

namespace db
{
class IDatabase;
class IConnection;
class IResultSet;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite-реализация репозитория истории изменений элементов.
 */
class SqliteItemHistoryRepository final : public IItemHistoryRepository
{
public:
    explicit SqliteItemHistoryRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemHistoryRepository() override = default;

    SqliteItemHistoryRepository(const SqliteItemHistoryRepository&) = delete;
    SqliteItemHistoryRepository& operator=(const SqliteItemHistoryRepository&) = delete;

    // IItemHistoryRepository
    ItemHistoriesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::ItemHistory> findById(int64_t id) override;
    std::vector<dto::ItemHistory> findByItemId(int64_t itemId) override;
    std::vector<dto::ItemHistory> findByUserId(int64_t userId) override;
    std::optional<dto::ItemHistory> findLastByItemId(int64_t itemId) override;
    int64_t create(const dto::ItemHistory& history) override;
    bool update(const dto::ItemHistory& history) override;
    bool remove(int64_t id) override;
    int64_t removeByItemId(int64_t itemId) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::ItemHistory mapRowToItemHistory(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
