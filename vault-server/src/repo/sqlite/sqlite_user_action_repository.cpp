#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_user_action_repository.h"

namespace server
{
namespace repositories
{

SqliteUserActionRepository::SqliteUserActionRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteUserActionRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteUserActionRepository::connection() const
{
    return m_database->connection();
}

dto::UserAction SqliteUserActionRepository::mapRowToUserAction(db::IResultSet& rs) const
{
    dto::UserAction action;
    action.id = rs.valueInt64("id");
    action.userId = rs.valueInt64("userId");
    action.caption = rs.valueString("caption");

    if (!rs.isNull("description"))
        action.description = rs.valueString("description");

    if (!rs.isNull("timestamp"))
    {
        int64_t timestamp = rs.valueInt64("timestamp");
        action.timestamp = common::secondsToTimePoint(timestamp);
    }

    return action;
}

UserActionsPage SqliteUserActionRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    std::vector<dto::UserAction> actions;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (dateFrom.has_value())
            whereClauses.push_back("timestamp >= :dateFrom");
        if (dateTo.has_value())
            whereClauses.push_back("timestamp <= :dateTo");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM UserAction" + whereClause
        );

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
            return { actions, totalCount };
        }

        // Получаем страницу с действиями
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, timestamp, caption, description "
            "FROM UserAction"
            + whereClause + " ORDER BY timestamp DESC LIMIT :limit OFFSET :offset"
        );

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
            actions.push_back(mapRowToUserAction(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка действий пользователя: " << e.what();
        throw;
    }

    return { actions, totalCount };
}

std::optional<dto::UserAction> SqliteUserActionRepository::findById(int64_t id)
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
            "SELECT id, userId, timestamp, caption, description "
            "FROM UserAction WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUserAction(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска действия по id: " << e.what();
        throw;
    }
}

std::vector<dto::UserAction> SqliteUserActionRepository::findByUserId(int64_t userId)
{
    std::vector<dto::UserAction> actions;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return actions;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, timestamp, caption, description "
            "FROM UserAction WHERE userId = :userId ORDER BY timestamp DESC"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            actions.push_back(mapRowToUserAction(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения действий пользователя: " << e.what();
        throw;
    }

    return actions;
}

int64_t SqliteUserActionRepository::create(const dto::UserAction& action)
{
    if (!action.userId.has_value() || !action.caption.has_value() || action.caption->empty())
    {
        LOG_WARN << "create: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        int64_t nowSeconds = common::timePointToSeconds(std::chrono::system_clock::now());

        auto stmt = conn->prepareStatement(
            "INSERT INTO UserAction (userId, timestamp, caption, description) "
            "VALUES (:userId, :timestamp, :caption, :description)"
        );

        stmt->bindInt64("userId", *action.userId);
        stmt->bindInt64("timestamp", action.timestamp.has_value() ? common::timePointToSeconds(*action.timestamp) : nowSeconds);
        stmt->bindString("caption", *action.caption);

        if (action.description.has_value())
            stmt->bindString("description", *action.description);
        else
            stmt->bindNull("description");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания действия: " << e.what();
        throw;
    }
}

bool SqliteUserActionRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM UserAction WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления действия: " << e.what();
        return false;
    }
}

bool SqliteUserActionRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM UserAction WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования действия: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
