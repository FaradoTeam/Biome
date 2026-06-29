#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_comment_document_repository.h"

namespace server
{
namespace repositories
{

SqliteCommentDocumentRepository::SqliteCommentDocumentRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteCommentDocumentRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteCommentDocumentRepository::connection() const
{
    return m_database->connection();
}

dto::CommentDocument SqliteCommentDocumentRepository::mapRowToCommentDocument(
    db::IResultSet& rs
) const
{
    dto::CommentDocument commentDocument;
    commentDocument.id = rs.valueInt64("id");
    commentDocument.commentId = rs.valueInt64("commentId");
    commentDocument.documentId = rs.valueInt64("documentId");
    return commentDocument;
}

std::pair<std::vector<dto::CommentDocument>, int64_t>
SqliteCommentDocumentRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> commentId,
    std::optional<int64_t> documentId
)
{
    std::vector<dto::CommentDocument> items;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (commentId.has_value())
            whereClauses.push_back("commentId = :commentId");
        if (documentId.has_value())
            whereClauses.push_back("documentId = :documentId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM CommentDocument" + whereStr
        );

        if (commentId.has_value())
            countStmt->bindInt64("commentId", *commentId);
        if (documentId.has_value())
            countStmt->bindInt64("documentId", *documentId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { items, totalCount };
        }

        // Получаем страницу
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, commentId, documentId FROM CommentDocument"
            + whereStr + " ORDER BY id LIMIT :limit OFFSET :offset"
        );

        if (commentId.has_value())
            stmt->bindInt64("commentId", *commentId);
        if (documentId.has_value())
            stmt->bindInt64("documentId", *documentId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            items.push_back(mapRowToCommentDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка CommentDocument: " << e.what();
        throw;
    }

    return { items, totalCount };
}

std::optional<dto::CommentDocument> SqliteCommentDocumentRepository::findById(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "findById: неверный идентификатор " << id;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, commentId, documentId FROM CommentDocument WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToCommentDocument(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска CommentDocument по id: " << e.what();
        throw;
    }
}

std::vector<dto::CommentDocument> SqliteCommentDocumentRepository::findByCommentId(
    int64_t commentId
)
{
    std::vector<dto::CommentDocument> items;

    if (commentId <= 0)
    {
        LOG_WARN << "findByCommentId: неверный commentId " << commentId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, commentId, documentId FROM CommentDocument "
            "WHERE commentId = :commentId ORDER BY documentId"
        );

        stmt->bindInt64("commentId", commentId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToCommentDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения CommentDocument по commentId: " << e.what();
        throw;
    }

    return items;
}

std::vector<dto::CommentDocument> SqliteCommentDocumentRepository::findByDocumentId(
    int64_t documentId
)
{
    std::vector<dto::CommentDocument> items;

    if (documentId <= 0)
    {
        LOG_WARN << "findByDocumentId: неверный documentId " << documentId;
        return items;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, commentId, documentId FROM CommentDocument "
            "WHERE documentId = :documentId ORDER BY commentId"
        );

        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            items.push_back(mapRowToCommentDocument(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения CommentDocument по documentId: " << e.what();
        throw;
    }

    return items;
}

std::optional<dto::CommentDocument> SqliteCommentDocumentRepository::findByCommentAndDocument(
    int64_t commentId,
    int64_t documentId
)
{
    if (commentId <= 0 || documentId <= 0)
    {
        LOG_WARN << "findByCommentAndDocument: неверные параметры";
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, commentId, documentId FROM CommentDocument "
            "WHERE commentId = :commentId AND documentId = :documentId LIMIT 1"
        );

        stmt->bindInt64("commentId", commentId);
        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToCommentDocument(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска CommentDocument по commentId и documentId: " << e.what();
        throw;
    }
}

bool SqliteCommentDocumentRepository::exists(int64_t commentId, int64_t documentId)
{
    if (commentId <= 0 || documentId <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM CommentDocument "
            "WHERE commentId = :commentId AND documentId = :documentId LIMIT 1"
        );

        stmt->bindInt64("commentId", commentId);
        stmt->bindInt64("documentId", documentId);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования CommentDocument: " << e.what();
        return false;
    }
}

int64_t SqliteCommentDocumentRepository::create(
    const dto::CommentDocument& commentDocument
)
{
    if (!commentDocument.commentId.has_value() || !commentDocument.documentId.has_value())
    {
        LOG_WARN << "create: отсутствуют обязательные поля";
        return 0;
    }

    // Проверяем уникальность пары
    if (exists(*commentDocument.commentId, *commentDocument.documentId))
    {
        LOG_WARN
            << "create: связь commentId=" << *commentDocument.commentId
            << ", documentId=" << *commentDocument.documentId << " уже существует";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "INSERT INTO CommentDocument (commentId, documentId) "
            "VALUES (:commentId, :documentId)"
        );

        stmt->bindInt64("commentId", *commentDocument.commentId);
        stmt->bindInt64("documentId", *commentDocument.documentId);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания CommentDocument: " << e.what();
        throw;
    }
}

bool SqliteCommentDocumentRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM CommentDocument WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления CommentDocument: " << e.what();
        return false;
    }
}

int64_t SqliteCommentDocumentRepository::removeByCommentId(int64_t commentId)
{
    if (commentId <= 0)
    {
        LOG_WARN << "removeByCommentId: неверный commentId " << commentId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM CommentDocument WHERE commentId = :commentId"
        );

        stmt->bindInt64("commentId", commentId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления CommentDocument по commentId: " << e.what();
        return 0;
    }
}

int64_t SqliteCommentDocumentRepository::removeByDocumentId(int64_t documentId)
{
    if (documentId <= 0)
    {
        LOG_WARN << "removeByDocumentId: неверный documentId " << documentId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM CommentDocument WHERE documentId = :documentId"
        );

        stmt->bindInt64("documentId", documentId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления CommentDocument по documentId: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
