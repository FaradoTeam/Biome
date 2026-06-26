#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../comment_repository.h"

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
 * @brief SQLite-реализация репозитория для работы с комментариями.
 */
class SqliteCommentRepository final : public ICommentRepository
{
public:
    explicit SqliteCommentRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteCommentRepository() override = default;

    SqliteCommentRepository(const SqliteCommentRepository&) = delete;
    SqliteCommentRepository& operator=(const SqliteCommentRepository&) = delete;

    CommentsPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::Comment> findById(int64_t id) override;
    std::vector<dto::Comment> findByItemId(
        int64_t itemId,
        bool sortAsc = true
    ) override;
    std::vector<dto::Comment> findByUserId(int64_t userId) override;
    int64_t create(const dto::Comment& comment) override;
    bool update(const dto::Comment& comment) override;
    bool remove(int64_t id) override;
    int64_t removeByItemId(int64_t itemId) override;
    bool exists(int64_t id) override;
    int64_t countByItemId(int64_t itemId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::Comment mapRowToComment(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
