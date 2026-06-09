#pragma once

#include <memory>

#include "../item_user_state_repository.h"

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
 * @brief SQLite-реализация репозитория истории состояний элементов.
 */
class SqliteItemUserStateRepository final : public IItemUserStateRepository
{
public:
    explicit SqliteItemUserStateRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteItemUserStateRepository() override = default;

    SqliteItemUserStateRepository(const SqliteItemUserStateRepository&) = delete;
    SqliteItemUserStateRepository& operator=(const SqliteItemUserStateRepository&) = delete;

    // IItemUserStateRepository
    ItemUserStatesPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt
    ) override;

    std::optional<dto::ItemUserState> findById(int64_t id) override;
    std::vector<dto::ItemUserState> findByItemId(int64_t itemId) override;
    std::vector<dto::ItemUserState> findByUserId(int64_t userId) override;
    std::optional<dto::ItemUserState> findLastByItemId(int64_t itemId) override;
    int64_t create(const dto::ItemUserState& state) override;
    bool update(const dto::ItemUserState& state) override;
    bool remove(int64_t id) override;
    int64_t removeByItemId(int64_t itemId) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::ItemUserState mapRowToItemUserState(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
