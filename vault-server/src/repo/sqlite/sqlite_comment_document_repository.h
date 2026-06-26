#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "../comment_document_repository.h"

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
 * @brief SQLite-реализация репозитория для связей комментариев с документами.
 */
class SqliteCommentDocumentRepository final : public ICommentDocumentRepository
{
public:
    explicit SqliteCommentDocumentRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteCommentDocumentRepository() override = default;

    SqliteCommentDocumentRepository(const SqliteCommentDocumentRepository&) = delete;
    SqliteCommentDocumentRepository& operator=(const SqliteCommentDocumentRepository&) = delete;

    std::pair<std::vector<dto::CommentDocument>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> commentId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override;

    std::optional<dto::CommentDocument> findById(int64_t id) override;
    std::vector<dto::CommentDocument> findByCommentId(int64_t commentId) override;
    std::vector<dto::CommentDocument> findByDocumentId(int64_t documentId) override;
    std::optional<dto::CommentDocument> findByCommentAndDocument(
        int64_t commentId,
        int64_t documentId
    ) override;
    bool exists(int64_t commentId, int64_t documentId) override;
    int64_t create(const dto::CommentDocument& commentDocument) override;
    bool remove(int64_t id) override;
    int64_t removeByCommentId(int64_t commentId) override;
    int64_t removeByDocumentId(int64_t documentId) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::CommentDocument mapRowToCommentDocument(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
