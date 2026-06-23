#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_team_message_repository.h"

namespace
{

dto::TeamMessage mapRowToTeamMessage(db::IResultSet& rs)
{
    dto::TeamMessage msg;
    msg.id = rs.valueInt64("id");
    msg.senderUserId = rs.valueInt64("senderUserId");
    msg.teamId = rs.valueInt64("teamId");

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

SqliteTeamMessageRepository::SqliteTeamMessageRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteTeamMessageRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteTeamMessageRepository::connection() const
{
    return m_database->connection();
}

TeamMessagesPage SqliteTeamMessageRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> teamId,
    std::optional<int64_t> senderUserId
)
{
    std::vector<dto::TeamMessage> messages;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        std::vector<std::string> whereClauses;
        if (teamId.has_value())
            whereClauses.push_back("teamId = :teamId");
        if (senderUserId.has_value())
            whereClauses.push_back("senderUserId = :senderUserId");

        std::string whereStr;
        if (!whereClauses.empty())
        {
            whereStr = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM TeamMessage" + whereStr
        );
        if (teamId.has_value())
            countStmt->bindInt64("teamId", *teamId);
        if (senderUserId.has_value())
            countStmt->bindInt64("senderUserId", *senderUserId);

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
        std::string selectSql = "SELECT id, senderUserId, teamId, creationTimestamp, content "
                                "FROM TeamMessage"
            + whereStr + " ORDER BY creationTimestamp DESC LIMIT :limit OFFSET :offset";

        auto stmt = conn->prepareStatement(selectSql);
        if (teamId.has_value())
            stmt->bindInt64("teamId", *teamId);
        if (senderUserId.has_value())
            stmt->bindInt64("senderUserId", *senderUserId);
        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToTeamMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка TeamMessage: " << e.what();
        throw;
    }

    return { messages, totalCount };
}

std::optional<dto::TeamMessage> SqliteTeamMessageRepository::findById(int64_t id)
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
            "SELECT id, senderUserId, teamId, creationTimestamp, content "
            "FROM TeamMessage WHERE id = :id"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToTeamMessage(*rs);
        }
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска TeamMessage по id: " << e.what();
        throw;
    }
}

std::vector<dto::TeamMessage> SqliteTeamMessageRepository::findByTeamId(int64_t teamId)
{
    std::vector<dto::TeamMessage> messages;

    if (teamId <= 0)
    {
        LOG_WARN << "findByTeamId: неверный teamId " << teamId;
        return messages;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, senderUserId, teamId, creationTimestamp, content "
            "FROM TeamMessage WHERE teamId = :teamId ORDER BY creationTimestamp DESC"
        );
        stmt->bindInt64("teamId", teamId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToTeamMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения сообщений команды: " << e.what();
        throw;
    }

    return messages;
}

std::vector<dto::TeamMessage> SqliteTeamMessageRepository::findBySenderAndTeam(
    int64_t senderUserId,
    int64_t teamId
)
{
    std::vector<dto::TeamMessage> messages;

    if (senderUserId <= 0 || teamId <= 0)
    {
        LOG_WARN << "findBySenderAndTeam: неверные параметры";
        return messages;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, senderUserId, teamId, creationTimestamp, content "
            "FROM TeamMessage "
            "WHERE senderUserId = :senderUserId AND teamId = :teamId "
            "ORDER BY creationTimestamp DESC"
        );
        stmt->bindInt64("senderUserId", senderUserId);
        stmt->bindInt64("teamId", teamId);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            messages.push_back(mapRowToTeamMessage(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения сообщений отправителя в команде: " << e.what();
        throw;
    }

    return messages;
}

int64_t SqliteTeamMessageRepository::create(const dto::TeamMessage& message)
{
    if (!message.senderUserId.has_value() || !message.teamId.has_value() || !message.content.has_value() || message.content->empty())
    {
        LOG_WARN << "createTeamMessage: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        int64_t timestamp = message.creationTimestamp.has_value()
            ? common::timePointToSeconds(*message.creationTimestamp)
            : common::timePointToSeconds(std::chrono::system_clock::now());

        auto stmt = conn->prepareStatement(
            "INSERT INTO TeamMessage (senderUserId, teamId, creationTimestamp, content) "
            "VALUES (:senderUserId, :teamId, :creationTimestamp, :content)"
        );

        stmt->bindInt64("senderUserId", *message.senderUserId);
        stmt->bindInt64("teamId", *message.teamId);
        stmt->bindInt64("creationTimestamp", timestamp);
        stmt->bindString("content", *message.content);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания TeamMessage: " << e.what();
        throw;
    }
}

bool SqliteTeamMessageRepository::update(const dto::TeamMessage& message)
{
    if (!message.id.has_value())
    {
        LOG_WARN << "updateTeamMessage: отсутствует ID";
        return false;
    }

    try
    {
        auto conn = connection();
        if (!message.content.has_value())
        {
            LOG_WARN << "updateTeamMessage: нет полей для обновления";
            return false;
        }

        auto stmt = conn->prepareStatement(
            "UPDATE TeamMessage SET content = :content WHERE id = :id"
        );
        stmt->bindString("content", *message.content);
        stmt->bindInt64("id", *message.id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления TeamMessage: " << e.what();
        return false;
    }
}

bool SqliteTeamMessageRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "removeTeamMessage: неверный ID " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM TeamMessage WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления TeamMessage: " << e.what();
        return false;
    }
}

bool SqliteTeamMessageRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM TeamMessage WHERE id = :id LIMIT 1"
        );
        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования TeamMessage: " << e.what();
        return false;
    }
}

int64_t SqliteTeamMessageRepository::removeByTeamId(int64_t teamId)
{
    if (teamId <= 0)
    {
        LOG_WARN << "removeByTeamId: неверный teamId " << teamId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM TeamMessage WHERE teamId = :teamId"
        );
        stmt->bindInt64("teamId", teamId);

        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления сообщений команды: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
