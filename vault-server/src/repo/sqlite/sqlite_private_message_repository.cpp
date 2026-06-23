#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_private_message_repository.h"

namespace
{

dto::PrivateMessage mapRowToPrivateMessage(db::IResultSet& rs)
{
    dto::PrivateMessage msg;
    msg.id = rs.valueInt64("id");
    msg.senderUserId = rs.valueInt64("senderUserId");
    msg.receiverUserId = rs.valueInt64("receiverUserId");
    msg.isViewed = rs.valueInt64("isViewed") != 0;

    if (!rs.isNull("content"))
        msg.content = rs.valueString("content");

    if (!rs.isNull("creationTimestamp"))
    {
        int64_t timestamp = rs.valueInt64("creationTimestamp");
        msg.creationTimestamp = common::secondsToTimePoint(timestamp);
    }

    return msg;
}

} // namespace

namespace server
{
namespace repositories
{

SqlitePrivateMessageRepository::SqlitePrivateMessageRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqlitePrivateMessageRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqlitePrivateMessageRepository::connection() const
{
    return m_database->connection();
}

PrivateMessagesPage SqlitePrivateMessageRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<bool> isViewed
)
{
    std::vector<dto::PrivateMessage> messages;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (userId.has_value())
        {
            whereClauses.push_back("(senderUserId = :userId OR receiverUserId = :userId)");
        }
        if (isViewed.has_value())
        {
            whereClauses.push_back("isViewed = :isViewed");
        }

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM PrivateMessage" + whereStr
        );
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (isViewed.has_value())
            countStmt->bindInt64("isViewed", *isViewed ? 1 : 0);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { messages, totalCount };
        }

        const int offset = (page - 1) * pageSize;
        std::string selectSql = "SELECT id, senderUserId, receiverUserId, creationTimestamp, content, isViewed "
                                "FROM PrivateMessage"
            + whereStr + " ORDER BY creationTimestamp DESC LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (isViewed.has_value())
            stmt->bindInt64("isViewed", *isViewed ? 1 : 0);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToPrivateMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка PrivateMessage: " << e.what();
        throw;
    }

    return { messages, totalCount };
}

std::optional<dto::PrivateMessage> SqlitePrivateMessageRepository::findById(int64_t id)
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
            "SELECT id, senderUserId, receiverUserId, creationTimestamp, content, isViewed "
            "FROM PrivateMessage WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToPrivateMessage(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска PrivateMessage по id: " << e.what();
        throw;
    }
}

std::vector<dto::PrivateMessage> SqlitePrivateMessageRepository::findConversation(
    int64_t userId1,
    int64_t userId2
)
{
    std::vector<dto::PrivateMessage> messages;

    if (userId1 <= 0 || userId2 <= 0)
    {
        LOG_WARN << "findConversation: неверные ID пользователей";
        return messages;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, senderUserId, receiverUserId, creationTimestamp, content, isViewed "
            "FROM PrivateMessage "
            "WHERE (senderUserId = :user1 AND receiverUserId = :user2) "
            "   OR (senderUserId = :user2 AND receiverUserId = :user1) "
            "ORDER BY creationTimestamp ASC"
        );
        stmt->bindInt64("user1", userId1);
        stmt->bindInt64("user2", userId2);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToPrivateMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения переписки: " << e.what();
        throw;
    }

    return messages;
}

std::vector<dto::PrivateMessage> SqlitePrivateMessageRepository::findBySender(int64_t senderUserId)
{
    std::vector<dto::PrivateMessage> messages;

    if (senderUserId <= 0)
    {
        LOG_WARN << "findBySender: неверный ID отправителя";
        return messages;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, senderUserId, receiverUserId, creationTimestamp, content, isViewed "
            "FROM PrivateMessage WHERE senderUserId = :senderUserId "
            "ORDER BY creationTimestamp DESC"
        );
        stmt->bindInt64("senderUserId", senderUserId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToPrivateMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения сообщений отправителя: " << e.what();
        throw;
    }

    return messages;
}

std::vector<dto::PrivateMessage> SqlitePrivateMessageRepository::findByReceiver(
    int64_t receiverUserId,
    bool onlyUnviewed
)
{
    std::vector<dto::PrivateMessage> messages;

    if (receiverUserId <= 0)
    {
        LOG_WARN << "findByReceiver: неверный ID получателя";
        return messages;
    }

    try
    {
        auto conn = connection();
        std::string sql = "SELECT id, senderUserId, receiverUserId, creationTimestamp, content, isViewed "
                          "FROM PrivateMessage WHERE receiverUserId = :receiverUserId";

        if (onlyUnviewed)
        {
            sql += " AND isViewed = 0";
        }
        sql += " ORDER BY creationTimestamp DESC";

        auto stmt = conn->prepareStatement(sql);
        stmt->bindInt64("receiverUserId", receiverUserId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToPrivateMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения сообщений получателя: " << e.what();
        throw;
    }

    return messages;
}

int64_t SqlitePrivateMessageRepository::create(const dto::PrivateMessage& message)
{
    if (!message.senderUserId.has_value() || !message.receiverUserId.has_value() || !message.content.has_value() || message.content->empty())
    {
        LOG_WARN << "createPrivateMessage: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        int64_t timestamp = message.creationTimestamp.has_value()
            ? common::timePointToSeconds(*message.creationTimestamp)
            : common::timePointToSeconds(std::chrono::system_clock::now());

        auto stmt = conn->prepareStatement(
            "INSERT INTO PrivateMessage (senderUserId, receiverUserId, creationTimestamp, content, isViewed) "
            "VALUES (:senderUserId, :receiverUserId, :creationTimestamp, :content, :isViewed)"
        );

        stmt->bindInt64("senderUserId", *message.senderUserId);
        stmt->bindInt64("receiverUserId", *message.receiverUserId);
        stmt->bindInt64("creationTimestamp", timestamp);
        stmt->bindString("content", *message.content);
        stmt->bindInt64("isViewed", message.isViewed.value_or(false) ? 1 : 0);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания PrivateMessage: " << e.what();
        throw;
    }
}

bool SqlitePrivateMessageRepository::update(const dto::PrivateMessage& message)
{
    if (!message.id.has_value())
    {
        LOG_WARN << "updatePrivateMessage: отсутствует ID";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;

        if (message.isViewed.has_value())
            setClauses.push_back("isViewed = :isViewed");
        if (message.content.has_value())
            setClauses.push_back("content = :content");

        if (setClauses.empty())
        {
            LOG_WARN << "updatePrivateMessage: нет полей для обновления";
            return false;
        }

        std::string sql = "UPDATE PrivateMessage SET " + boost::algorithm::join(setClauses, ", ") + " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);
        if (message.isViewed.has_value())
            stmt->bindInt64("isViewed", *message.isViewed ? 1 : 0);
        if (message.content.has_value())
            stmt->bindString("content", *message.content);
        stmt->bindInt64("id", *message.id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления PrivateMessage: " << e.what();
        return false;
    }
}

bool SqlitePrivateMessageRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removePrivateMessage: неверный ID " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM PrivateMessage WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления PrivateMessage: " << e.what();
        return false;
    }
}

bool SqlitePrivateMessageRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM PrivateMessage WHERE id = :id LIMIT 1"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования PrivateMessage: " << e.what();
        return false;
    }
}

int64_t SqlitePrivateMessageRepository::markAllAsViewed(
    int64_t senderUserId,
    int64_t receiverUserId
)
{
    if (senderUserId <= 0 || receiverUserId <= 0)
    {
        LOG_WARN << "markAllAsViewed: неверные ID пользователей";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "UPDATE PrivateMessage SET isViewed = 1 "
            "WHERE senderUserId = :senderUserId "
            "  AND receiverUserId = :receiverUserId "
            "  AND isViewed = 0"
        );
        stmt->bindInt64("senderUserId", senderUserId);
        stmt->bindInt64("receiverUserId", receiverUserId);

        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка отметки всех сообщений как прочитанных: " << e.what();
        return 0;
    }
}

int64_t SqlitePrivateMessageRepository::countUnviewed(int64_t userId)
{
    if (userId <= 0)
    {
        LOG_WARN << "countUnviewed: неверный ID пользователя";
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM PrivateMessage "
            "WHERE receiverUserId = :userId AND isViewed = 0"
        );
        stmt->bindInt64("userId", userId);

        auto rs = stmt->executeQuery();
        if (rs->next())
        {
            return rs->valueInt64(0);
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка подсчёта непрочитанных сообщений: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
