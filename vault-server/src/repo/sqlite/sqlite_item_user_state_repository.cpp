#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/types.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_user_state_repository.h"

namespace server
{
namespace repositories
{

dto::ItemUserState SqliteItemUserStateRepository::mapRowToItemUserState(db::IResultSet& rs) const
{
    dto::ItemUserState state;
    state.id = rs.valueInt64("id");
    state.itemId = rs.valueInt64("itemId");
    state.userId = rs.valueInt64("userId");
    state.stateId = rs.valueInt64("stateId");

    if (!rs.isNull("comment"))
        state.comment = rs.valueString("comment");

    if (!rs.isNull("timestamp"))
    {
        const int64_t timestamp = rs.valueInt64("timestamp");
        state.timestamp = common::secondsToTimePoint(timestamp);
    }

    return state;
}

SqliteItemUserStateRepository::SqliteItemUserStateRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemUserStateRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteItemUserStateRepository::connection() const
{
    return m_database->connection();
}

ItemUserStatesPage SqliteItemUserStateRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemId,
    std::optional<int64_t> userId
)
{
    std::vector<dto::ItemUserState> states;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Строим WHERE clause
        std::vector<std::string> whereConditions;
        if (itemId.has_value())
            whereConditions.push_back("itemId = :itemId");
        if (userId.has_value())
            whereConditions.push_back("userId = :userId");

        std::string whereClause;
        if (!whereConditions.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereConditions, " AND ");
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM ItemUserState" + whereClause
        );

        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0)
        {
            return { states, totalCount };
        }

        // 2. Вычисляем offset
        const int offset = (page - 1) * pageSize;

        // Если offset выходит за пределы, возвращаем пустой результат
        if (offset >= totalCount)
        {
            return { states, totalCount };
        }

        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, stateId, comment, timestamp "
            "FROM ItemUserState"
            + whereClause + " ORDER BY timestamp DESC LIMIT :limit OFFSET :offset"
        );

        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            states.push_back(mapRowToItemUserState(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка ItemUserState: " << e.what();
        throw;
    }

    return { states, totalCount };
}

std::optional<dto::ItemUserState> SqliteItemUserStateRepository::findById(int64_t id)
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
            "SELECT id, itemId, userId, stateId, comment, timestamp "
            "FROM ItemUserState WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemUserState(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска ItemUserState по id: " << e.what();
        throw;
    }
}

std::vector<dto::ItemUserState> SqliteItemUserStateRepository::findByItemId(int64_t itemId)
{
    std::vector<dto::ItemUserState> states;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return states;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, stateId, comment, timestamp "
            "FROM ItemUserState WHERE itemId = :itemId ORDER BY timestamp DESC"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            states.push_back(mapRowToItemUserState(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemUserState по itemId: " << e.what();
        throw;
    }

    return states;
}

std::vector<dto::ItemUserState> SqliteItemUserStateRepository::findByUserId(int64_t userId)
{
    std::vector<dto::ItemUserState> states;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return states;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, stateId, comment, timestamp "
            "FROM ItemUserState WHERE userId = :userId ORDER BY timestamp DESC"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            states.push_back(mapRowToItemUserState(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemUserState по userId: " << e.what();
        throw;
    }

    return states;
}

std::optional<dto::ItemUserState> SqliteItemUserStateRepository::findLastByItemId(int64_t itemId)
{
    if (itemId <= 0)
    {
        LOG_WARN << "findLastByItemId: неверный itemId " << itemId;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, stateId, comment, timestamp "
            "FROM ItemUserState WHERE itemId = :itemId "
            "ORDER BY timestamp DESC LIMIT 1"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemUserState(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения последнего ItemUserState для элемента: " << e.what();
        throw;
    }
}

int64_t SqliteItemUserStateRepository::create(const dto::ItemUserState& state)
{
    if (!state.itemId.has_value() || !state.userId.has_value() || !state.stateId.has_value())
    {
        LOG_WARN << "Создание ItemUserState: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO ItemUserState (itemId, userId, stateId, comment, timestamp) "
            "VALUES (:itemId, :userId, :stateId, :comment, :timestamp)"
        );

        stmt->bindInt64("itemId", *state.itemId);
        stmt->bindInt64("userId", *state.userId);
        stmt->bindInt64("stateId", *state.stateId);

        if (state.comment.has_value())
            stmt->bindString("comment", *state.comment);
        else
            stmt->bindNull("comment");

        const int64_t timestamp = state.timestamp.has_value()
            ? common::timePointToSeconds(*state.timestamp)
            : common::timePointToSeconds(std::chrono::system_clock::now());

        stmt->bindInt64("timestamp", timestamp);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания ItemUserState: " << e.what();
        throw;
    }
}

bool SqliteItemUserStateRepository::update(const dto::ItemUserState& state)
{
    if (!state.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID записи";
        return false;
    }

    LOG_DEBUG << "update: обновление записи id=" << *state.id;

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE ItemUserState SET ";

        if (state.itemId.has_value())
        {
            setClauses.push_back("itemId = :itemId");
            LOG_DEBUG << "update: будет обновлён itemId=" << *state.itemId;
        }
        if (state.userId.has_value())
        {
            setClauses.push_back("userId = :userId");
            LOG_DEBUG << "update: будет обновлён userId=" << *state.userId;
        }
        if (state.stateId.has_value())
        {
            setClauses.push_back("stateId = :stateId");
            LOG_DEBUG << "update: будет обновлён stateId=" << *state.stateId;
        }
        if (state.comment.has_value())
        {
            setClauses.push_back("comment = :comment");
            LOG_DEBUG << "update: будет обновлён comment=" << *state.comment;
        }
        if (state.timestamp.has_value())
        {
            setClauses.push_back("timestamp = :timestamp");
            LOG_DEBUG << "update: будет обновлён timestamp";
        }

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        LOG_DEBUG << "update: SQL=" << sql;

        auto stmt = conn->prepareStatement(sql);

        if (state.itemId.has_value())
            stmt->bindInt64("itemId", *state.itemId);
        if (state.userId.has_value())
            stmt->bindInt64("userId", *state.userId);
        if (state.stateId.has_value())
            stmt->bindInt64("stateId", *state.stateId);
        if (state.comment.has_value())
            stmt->bindString("comment", *state.comment);
        if (state.timestamp.has_value())
            stmt->bindInt64("timestamp", common::timePointToSeconds(*state.timestamp));

        stmt->bindInt64("id", *state.id);

        const int64_t affected = stmt->execute();
        LOG_DEBUG << "update: затронуто строк=" << affected;
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления ItemUserState: " << e.what();
        return false;
    }
}

bool SqliteItemUserStateRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ItemUserState WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemUserState: " << e.what();
        return false;
    }
}

int64_t SqliteItemUserStateRepository::removeByItemId(int64_t itemId)
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
            "DELETE FROM ItemUserState WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemUserState по itemId: " << e.what();
        return 0;
    }
}

bool SqliteItemUserStateRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemUserState WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования ItemUserState: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
