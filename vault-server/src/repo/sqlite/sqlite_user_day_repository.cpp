#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_user_day_repository.h"

namespace server
{
namespace repositories
{

SqliteUserDayRepository::SqliteUserDayRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteUserDayRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteUserDayRepository::connection() const
{
    return m_database->connection();
}

dto::UserDay SqliteUserDayRepository::mapRowToUserDay(db::IResultSet& rs) const
{
    dto::UserDay day;
    day.id = rs.valueInt64("id");
    day.userId = rs.valueInt64("userId");

    if (!rs.isNull("date"))
    {
        day.date = common::secondsToTimePoint(rs.valueInt64("date"));
    }

    day.isWorkDay = rs.valueInt64("isWorkDay") != 0;

    if (!rs.isNull("beginWorkTime"))
        day.beginWorkTime = rs.valueString("beginWorkTime");

    if (!rs.isNull("endWorkTime"))
        day.endWorkTime = rs.valueString("endWorkTime");

    if (!rs.isNull("breakDuration"))
        day.breakDuration = rs.valueInt64("breakDuration");

    if (!rs.isNull("description"))
        day.description = rs.valueString("description");

    return day;
}

UserDaysPage SqliteUserDayRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<common::DateTime> dateFrom,
    std::optional<common::DateTime> dateTo
)
{
    std::vector<dto::UserDay> days;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (dateFrom.has_value())
            whereClauses.push_back("date >= :dateFrom");
        if (dateTo.has_value())
            whereClauses.push_back("date <= :dateTo");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM UserDay" + whereClause
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
            return { days, totalCount };
        }

        // Получаем страницу
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description "
            "FROM UserDay"
            + whereClause + " ORDER BY date DESC LIMIT :limit OFFSET :offset"
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
            days.push_back(mapRowToUserDay(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка пользовательских дней: " << e.what();
        throw;
    }

    return { days, totalCount };
}

std::optional<dto::UserDay> SqliteUserDayRepository::findById(int64_t id)
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
            "SELECT id, userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description "
            "FROM UserDay WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUserDay(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска пользовательского дня по id: " << e.what();
        throw;
    }
}

std::optional<dto::UserDay> SqliteUserDayRepository::findByUserAndDate(
    int64_t userId,
    const common::DateTime& date
)
{
    if (userId <= 0)
    {
        LOG_WARN << "findByUserAndDate: неверный userId " << userId;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description "
            "FROM UserDay WHERE userId = :userId AND date = :date"
        );

        stmt->bindInt64("userId", userId);
        stmt->bindInt64("date", common::timePointToSeconds(date));
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUserDay(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска пользовательского дня по userId и дате: " << e.what();
        throw;
    }
}

std::vector<dto::UserDay> SqliteUserDayRepository::findByUserId(int64_t userId)
{
    std::vector<dto::UserDay> days;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return days;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description "
            "FROM UserDay WHERE userId = :userId ORDER BY date DESC"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            days.push_back(mapRowToUserDay(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения пользовательских дней по userId: " << e.what();
        throw;
    }

    return days;
}

int64_t SqliteUserDayRepository::create(const dto::UserDay& userDay)
{
    if (!userDay.userId.has_value() || !userDay.date.has_value())
    {
        LOG_WARN << "create: отсутствуют обязательные поля (userId, date)";
        return 0;
    }

    // Проверяем уникальность пары (userId, date)
    if (findByUserAndDate(*userDay.userId, *userDay.date).has_value())
    {
        LOG_WARN
            << "create: пользовательский день для userId=" << *userDay.userId
            << " и даты уже существует";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO UserDay (userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description) "
            "VALUES (:userId, :date, :isWorkDay, :beginWorkTime, :endWorkTime, "
            ":breakDuration, :description)"
        );

        stmt->bindInt64("userId", *userDay.userId);
        stmt->bindInt64("date", common::timePointToSeconds(*userDay.date));
        stmt->bindInt64("isWorkDay", userDay.isWorkDay.value_or(false) ? 1 : 0);

        if (userDay.beginWorkTime.has_value())
            stmt->bindString("beginWorkTime", *userDay.beginWorkTime);
        else
            stmt->bindNull("beginWorkTime");

        if (userDay.endWorkTime.has_value())
            stmt->bindString("endWorkTime", *userDay.endWorkTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", userDay.breakDuration.value_or(0));

        if (userDay.description.has_value())
            stmt->bindString("description", *userDay.description);
        else
            stmt->bindNull("description");

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания пользовательского дня: " << e.what();
        throw;
    }
}

bool SqliteUserDayRepository::update(const dto::UserDay& userDay)
{
    if (!userDay.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID пользовательского дня";
        return false;
    }

    try
    {
        auto conn = connection();

        // Проверяем, можно ли изменить userId или date без конфликта
        auto existing = findById(*userDay.id);
        if (!existing.has_value())
        {
            LOG_WARN << "update: пользовательский день не найден, id=" << *userDay.id;
            return false;
        }

        int64_t newUserId = userDay.userId.has_value() ? *userDay.userId : *existing->userId;
        common::DateTime newDate = userDay.date.has_value() ? *userDay.date : *existing->date;

        if ((userDay.userId.has_value() && *userDay.userId != *existing->userId) || (userDay.date.has_value() && *userDay.date != *existing->date))
        {
            auto conflicting = findByUserAndDate(newUserId, newDate);
            if (conflicting.has_value() && *conflicting->id != *userDay.id)
            {
                LOG_WARN
                    << "update: конфликт, день для userId=" << newUserId
                    << " и даты уже существует";
                return false;
            }
        }

        auto stmt = conn->prepareStatement(
            "UPDATE UserDay SET "
            "userId = :userId, "
            "date = :date, "
            "isWorkDay = :isWorkDay, "
            "beginWorkTime = :beginWorkTime, "
            "endWorkTime = :endWorkTime, "
            "breakDuration = :breakDuration, "
            "description = :description "
            "WHERE id = :id"
        );

        stmt->bindInt64("id", *userDay.id);
        stmt->bindInt64("userId", newUserId);
        stmt->bindInt64("date", common::timePointToSeconds(newDate));
        stmt->bindInt64("isWorkDay", userDay.isWorkDay.value_or(*existing->isWorkDay) ? 1 : 0);

        std::string beginTime = userDay.beginWorkTime.has_value()
            ? *userDay.beginWorkTime
            : existing->beginWorkTime.value_or("");
        if (!beginTime.empty())
            stmt->bindString("beginWorkTime", beginTime);
        else
            stmt->bindNull("beginWorkTime");

        std::string endTime = userDay.endWorkTime.has_value()
            ? *userDay.endWorkTime
            : existing->endWorkTime.value_or("");
        if (!endTime.empty())
            stmt->bindString("endWorkTime", endTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", userDay.breakDuration.value_or(*existing->breakDuration));

        std::string desc = userDay.description.has_value()
            ? *userDay.description
            : existing->description.value_or("");
        if (!desc.empty())
            stmt->bindString("description", desc);
        else
            stmt->bindNull("description");

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления пользовательского дня: " << e.what();
        return false;
    }
}

bool SqliteUserDayRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM UserDay WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления пользовательского дня: " << e.what();
        return false;
    }
}

int64_t SqliteUserDayRepository::removeByUserId(int64_t userId)
{
    if (userId <= 0)
    {
        LOG_WARN << "removeByUserId: неверный userId " << userId;
        return 0;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "DELETE FROM UserDay WHERE userId = :userId"
        );

        stmt->bindInt64("userId", userId);
        return stmt->execute();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления пользовательских дней по userId: " << e.what();
        return 0;
    }
}

} // namespace repositories
} // namespace server
