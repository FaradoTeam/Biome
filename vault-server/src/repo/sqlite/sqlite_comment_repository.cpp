#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_comment_repository.h"

namespace server
{
namespace repositories
{

SqliteCommentRepository::SqliteCommentRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteCommentRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteCommentRepository::connection() const
{
    return m_database->connection();
}

dto::Comment SqliteCommentRepository::mapRowToComment(db::IResultSet& rs) const
{
    dto::Comment comment;
    comment.id = rs.valueInt64("id");
    comment.userId = rs.valueInt64("userId");
    comment.itemId = rs.valueInt64("itemId");
    comment.content = rs.valueString("content");

    if (!rs.isNull("createdAt"))
    {
        int64_t timestamp = rs.valueInt64("createdAt");
        comment.createdAt = common::secondsToTimePoint(timestamp);
    }

    return comment;
}

CommentsPage SqliteCommentRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemId,
    std::optional<int64_t> userId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    std::vector<dto::Comment> comments;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (itemId.has_value())
            whereClauses.push_back("itemId = :itemId");
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (dateFrom.has_value())
            whereClauses.push_back("createdAt >= :dateFrom");
        if (dateTo.has_value())
            whereClauses.push_back("createdAt <= :dateTo");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Comment" + whereStr
        );

        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (dateFrom.has_value())
            countStmt->bindInt64("dateFrom", common::timePointToSeconds(*dateFrom));
        if (dateTo.has_value())
            countStmt->bindInt64("dateTo", common::timePointToSeconds(*dateTo));

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { comments, totalCount };
        }

        // Получаем страницу
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId, createdAt, content, searchContent "
            "FROM Comment"
            + whereStr + " ORDER BY createdAt DESC LIMIT :limit OFFSET :offset"
        );

        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (dateFrom.has_value())
            stmt->bindInt64("dateFrom", common::timePointToSeconds(*dateFrom));
        if (dateTo.has_value())
            stmt->bindInt64("dateTo", common::timePointToSeconds(*dateTo));

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            comments.push_back(mapRowToComment(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка комментариев: " << e.what();
        throw;
    }

    return { comments, totalCount };
}

std::optional<dto::Comment> SqliteCommentRepository::findById(int64_t id)
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
            "SELECT id, userId, itemId, createdAt, content, searchContent "
            "FROM Comment WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToComment(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска комментария по id: " << e.what();
        throw;
    }
}

std::vector<dto::Comment> SqliteCommentRepository::findByItemId(
    int64_t itemId,
    bool sortAsc
)
{
    std::vector<dto::Comment> comments;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return comments;
    }

    try
    {
        auto conn = connection();

        std::string order = sortAsc ? "ASC" : "DESC";
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId, createdAt, content, searchContent "
            "FROM Comment WHERE itemId = :itemId ORDER BY createdAt "
            + order
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            comments.push_back(mapRowToComment(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения комментариев по itemId: " << e.what();
        throw;
    }

    return comments;
}

std::vector<dto::Comment> SqliteCommentRepository::findByUserId(int64_t userId)
{
    std::vector<dto::Comment> comments;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return comments;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, itemId, createdAt, content, searchContent "
            "FROM Comment WHERE userId = :userId ORDER BY createdAt DESC"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            comments.push_back(mapRowToComment(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения комментариев по userId: " << e.what();
        throw;
    }

    return comments;
}

int64_t SqliteCommentRepository::create(const dto::Comment& comment)
{
    if (!comment.userId.has_value() || !comment.itemId.has_value() || !comment.content.has_value())
    {
        LOG_WARN << "create: отсутствуют обязательные поля";
        return 0;
    }

    if (comment.content->empty())
    {
        LOG_WARN << "create: content не может быть пустым";
        return 0;
    }

    try
    {
        auto conn = connection();

        int64_t nowSeconds = common::timePointToSeconds(std::chrono::system_clock::now());

        auto stmt = conn->prepareStatement(
            "INSERT INTO Comment (userId, itemId, createdAt, content, searchContent) "
            "VALUES (:userId, :itemId, :createdAt, :content, :searchContent)"
        );

        stmt->bindInt64("userId", *comment.userId);
        stmt->bindInt64("itemId", *comment.itemId);
        stmt->bindInt64("createdAt", nowSeconds);
        stmt->bindString("content", *comment.content);
        stmt->bindString("searchContent", common::toLowerCase(*comment.content));

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания комментария: " << e.what();
        throw;
    }
}

bool SqliteCommentRepository::update(const dto::Comment& comment)
{
    if (!comment.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID комментария";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE Comment SET ";

        if (comment.content.has_value())
        {
            if (comment.content->empty())
            {
                LOG_WARN << "update: content не может быть пустым";
                return false;
            }
            setClauses.push_back("content = :content");
            setClauses.push_back("searchContent = :searchContent");
        }

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (comment.content.has_value())
        {
            stmt->bindString("content", *comment.content);
            stmt->bindString("searchContent", common::toLowerCase(*comment.content));
        }

        stmt->bindInt64("id", *comment.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления комментария: " << e.what();
        return false;
    }
}

bool SqliteCommentRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM Comment WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления комментария: " << e.what();
        return false;
    }
}

int64_t SqliteCommentRepository::removeByItemId(int64_t itemId)
{
    if (itemId <= 0)
    {
        LOG_WARN << "removeByItemId: неверный itemId " << itemId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM Comment WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления комментариев по itemId: " << e.what();
        return 0;
    }
}

bool SqliteCommentRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM Comment WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования комментария: " << e.what();
        return false;
    }
}

int64_t SqliteCommentRepository::countByItemId(int64_t itemId)
{
    if (itemId <= 0)
        return 0;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM Comment WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return rs->valueInt64(0);
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка подсчёта комментариев: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
