#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/time_helpers.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_item_history_repository.h"

namespace server
{
namespace repositories
{

dto::ItemHistory SqliteItemHistoryRepository::mapRowToItemHistory(db::IResultSet& rs) const
{
    dto::ItemHistory history;
    history.id = rs.valueInt64("id");
    history.itemId = rs.valueInt64("itemId");
    history.userId = rs.valueInt64("userId");

    if (!rs.isNull("diff"))
        history.diff = rs.valueString("diff");

    if (!rs.isNull("timestamp"))
    {
        const int64_t timestamp = rs.valueInt64("timestamp");
        history.timestamp = dto::secondsToTimePoint(timestamp);
    }

    return history;
}

SqliteItemHistoryRepository::SqliteItemHistoryRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteItemHistoryRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteItemHistoryRepository::connection() const
{
    return m_database->connection();
}

ItemHistoriesPage SqliteItemHistoryRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> itemId,
    std::optional<int64_t> userId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    std::vector<dto::ItemHistory> histories;
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
        if (dateFrom.has_value())
            whereConditions.push_back("timestamp >= :dateFrom");
        if (dateTo.has_value())
            whereConditions.push_back("timestamp <= :dateTo");

        std::string whereClause;
        if (!whereConditions.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereConditions, " AND ");
        }

        // 1. Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM ItemHistory" + whereClause
        );

        if (itemId.has_value())
            countStmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (dateFrom.has_value())
            countStmt->bindInt64("dateFrom", dto::timePointToSeconds(*dateFrom));
        if (dateTo.has_value())
            countStmt->bindInt64("dateTo", dto::timePointToSeconds(*dateTo));

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0)
        {
            return { histories, totalCount };
        }

        // 2. Вычисляем offset
        const int offset = (page - 1) * pageSize;

        // Если offset выходит за пределы, возвращаем пустой результат
        if (offset >= totalCount)
        {
            return { histories, totalCount };
        }

        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, diff, timestamp "
            "FROM ItemHistory"
            + whereClause + " ORDER BY timestamp DESC LIMIT :limit OFFSET :offset"
        );

        if (itemId.has_value())
            stmt->bindInt64("itemId", *itemId);
        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (dateFrom.has_value())
            stmt->bindInt64("dateFrom", dto::timePointToSeconds(*dateFrom));
        if (dateTo.has_value())
            stmt->bindInt64("dateTo", dto::timePointToSeconds(*dateTo));

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            histories.push_back(mapRowToItemHistory(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка ItemHistory: " << e.what();
        throw;
    }

    return { histories, totalCount };
}

std::optional<dto::ItemHistory> SqliteItemHistoryRepository::findById(int64_t id)
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
            "SELECT id, itemId, userId, diff, timestamp "
            "FROM ItemHistory WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemHistory(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска ItemHistory по id: " << e.what();
        throw;
    }
}

std::vector<dto::ItemHistory> SqliteItemHistoryRepository::findByItemId(int64_t itemId)
{
    std::vector<dto::ItemHistory> histories;

    if (itemId <= 0)
    {
        LOG_WARN << "findByItemId: неверный itemId " << itemId;
        return histories;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, diff, timestamp "
            "FROM ItemHistory WHERE itemId = :itemId ORDER BY timestamp DESC"
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            histories.push_back(mapRowToItemHistory(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemHistory по itemId: " << e.what();
        throw;
    }

    return histories;
}

std::vector<dto::ItemHistory> SqliteItemHistoryRepository::findByUserId(int64_t userId)
{
    std::vector<dto::ItemHistory> histories;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return histories;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, itemId, userId, diff, timestamp "
            "FROM ItemHistory WHERE userId = :userId ORDER BY timestamp DESC"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            histories.push_back(mapRowToItemHistory(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения ItemHistory по userId: " << e.what();
        throw;
    }

    return histories;
}

std::optional<dto::ItemHistory> SqliteItemHistoryRepository::findLastByItemId(int64_t itemId)
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
            "SELECT id, itemId, userId, diff, timestamp "
            "FROM ItemHistory WHERE itemId = :itemId "
            "ORDER BY timestamp DESC LIMIT 1" // <-- DESC, а не ASC!
        );

        stmt->bindInt64("itemId", itemId);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToItemHistory(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения последнего ItemHistory для элемента: " << e.what();
        throw;
    }
}

int64_t SqliteItemHistoryRepository::create(const dto::ItemHistory& history)
{
    if (!history.itemId.has_value() || !history.userId.has_value())
    {
        LOG_WARN << "Создание ItemHistory: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO ItemHistory (itemId, userId, diff, timestamp) "
            "VALUES (:itemId, :userId, :diff, :timestamp)"
        );

        stmt->bindInt64("itemId", *history.itemId);
        stmt->bindInt64("userId", *history.userId);

        if (history.diff.has_value())
            stmt->bindString("diff", *history.diff);
        else
            stmt->bindNull("diff");

        const int64_t timestamp = history.timestamp.has_value()
            ? dto::timePointToSeconds(*history.timestamp)
            : dto::timePointToSeconds(std::chrono::system_clock::now());

        stmt->bindInt64("timestamp", timestamp);

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания ItemHistory: " << e.what();
        throw;
    }
}

bool SqliteItemHistoryRepository::update(const dto::ItemHistory& history)
{
    if (!history.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID записи";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE ItemHistory SET ";

        if (history.itemId.has_value())
            setClauses.push_back("itemId = :itemId");

        if (history.userId.has_value())
            setClauses.push_back("userId = :userId");

        if (history.diff.has_value())
            setClauses.push_back("diff = :diff");

        if (history.timestamp.has_value())
            setClauses.push_back("timestamp = :timestamp");

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (history.itemId.has_value())
            stmt->bindInt64("itemId", *history.itemId);

        if (history.userId.has_value())
            stmt->bindInt64("userId", *history.userId);

        if (history.diff.has_value())
            stmt->bindString("diff", *history.diff);

        if (history.timestamp.has_value())
            stmt->bindInt64("timestamp", dto::timePointToSeconds(*history.timestamp));

        stmt->bindInt64("id", *history.id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления ItemHistory: " << e.what();
        return false;
    }
}

bool SqliteItemHistoryRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM ItemHistory WHERE id = :id");
        stmt->bindInt64("id", id);

        const int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemHistory: " << e.what();
        return false;
    }
}

int64_t SqliteItemHistoryRepository::removeByItemId(int64_t itemId)
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
            "DELETE FROM ItemHistory WHERE itemId = :itemId"
        );

        stmt->bindInt64("itemId", itemId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления ItemHistory по itemId: " << e.what();
        return 0;
    }
}

bool SqliteItemHistoryRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM ItemHistory WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования ItemHistory: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
