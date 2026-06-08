#pragma once

#include <memory>

#include "../item_repository.h"

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
 * @brief SQLite-реализация репозитория элементов.
 */
class SqliteItemRepository final : public IItemRepository
{
public:
    explicit SqliteItemRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemRepository() override = default;

    SqliteItemRepository(const SqliteItemRepository&) = delete;
    SqliteItemRepository& operator=(const SqliteItemRepository&) = delete;

    // IItemRepository
    ItemsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt,
        const std::string& searchCaption = "",
        const std::vector<int64_t>& projectIds = {}
    ) override;

    std::optional<dto::Item> findById(int64_t id) override;
    int64_t create(const dto::Item& item) override;
    bool update(const dto::Item& item) override;
    bool softDelete(int64_t id) override;
    bool restore(int64_t id) override;
    bool hardDelete(int64_t id) override;
    bool exists(int64_t id) override;
    std::vector<dto::Item> findChildren(int64_t parentId, bool includeDeleted = false) override;
    std::vector<dto::Item> findRootItems(int64_t phaseId, bool includeDeleted = false) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
