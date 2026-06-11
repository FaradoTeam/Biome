#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "repo/item_link_repository.h"

namespace db
{
class IDatabase;
class IConnection;
} // namespace db

namespace server::repositories
{

/**
 * @brief SQLite-реализация репозитория для связей элементов.
 */
class SqliteItemLinkRepository final : public IItemLinkRepository
{
public:
    explicit SqliteItemLinkRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemLinkRepository() override = default;

    SqliteItemLinkRepository(const SqliteItemLinkRepository&) = delete;
    SqliteItemLinkRepository& operator=(const SqliteItemLinkRepository&) = delete;

    std::pair<std::vector<dto::ItemLink>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> linkTypeId = std::nullopt,
        std::optional<int64_t> sourceItemId = std::nullopt,
        std::optional<int64_t> destinationItemId = std::nullopt
    ) override;

    std::optional<dto::ItemLink> findById(int64_t id) override;
    std::vector<dto::ItemLink> findByItemId(int64_t itemId) override;
    std::vector<dto::ItemLink> findByLinkTypeId(int64_t linkTypeId) override;
    int64_t create(const dto::ItemLink& itemLink) override;
    bool update(const dto::ItemLink& itemLink) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;
    bool existsByTriple(
        int64_t linkTypeId,
        int64_t sourceItemId,
        int64_t destinationItemId
    ) override;
    int64_t removeByItemId(int64_t itemId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace server::repositories
