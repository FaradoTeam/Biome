#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/log/log.h"
#include "common/types.h"

#include "storage/idatabase.h"

#include "sqlite_special_day_repository.h"

namespace server
{
namespace repositories
{

SqliteSpecialDayRepository::SqliteSpecialDayRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteSpecialDayRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteSpecialDayRepository::connection() const
{
    return m_database->connection();
}

dto::SpecialDay SqliteSpecialDayRepository::mapRowToSpecialDay(db::IResultSet& rs) const
{
    dto::SpecialDay day;
    day.id = rs.valueInt64("id");

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

    return day;
}

SpecialDaysPage SqliteSpecialDayRepository::findAll(
    int page,
    int pageSize,
    std::optional<int> year,
    std::optional<int> month
)
{
    std::vector<dto::SpecialDay> days;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (year.has_value())
        {
            whereClauses.push_back("strftime('%Y', datetime(date, 'unixepoch')) = :year");
        }
        if (month.has_value())
        {
            whereClauses.push_back("strftime('%m', datetime(date, 'unixepoch')) = :month");
        }

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM SpecialDay" + whereClause
        );

        if (year.has_value())
        {
            countStmt->bindString("year", std::to_string(*year));
        }
        if (month.has_value())
        {
            std::string monthStr = std::to_string(*month);
            if (monthStr.length() == 1)
            {
                monthStr = "0" + monthStr;
            }
            countStmt->bindString("month", monthStr);
        }

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
            "SELECT id, date, isWorkDay, beginWorkTime, endWorkTime, breakDuration "
            "FROM SpecialDay"
            + whereClause + " ORDER BY date LIMIT :limit OFFSET :offset"
        );

        if (year.has_value())
        {
            stmt->bindString("year", std::to_string(*year));
        }
        if (month.has_value())
        {
            std::string monthStr = std::to_string(*month);
            if (monthStr.length() == 1)
            {
                monthStr = "0" + monthStr;
            }
            stmt->bindString("month", monthStr);
        }

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            days.push_back(mapRowToSpecialDay(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка особых дней: " << e.what();
        throw;
    }

    return { days, totalCount };
}

std::optional<dto::SpecialDay> SqliteSpecialDayRepository::findById(int64_t id)
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
            "SELECT id, date, isWorkDay, beginWorkTime, endWorkTime, breakDuration "
            "FROM SpecialDay WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToSpecialDay(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска особого дня по id: " << e.what();
        throw;
    }
}

std::optional<dto::SpecialDay> SqliteSpecialDayRepository::findByDate(
    const common::DateTime& date
)
{
    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, date, isWorkDay, beginWorkTime, endWorkTime, breakDuration "
            "FROM SpecialDay WHERE date = :date"
        );

        stmt->bindInt64("date", common::timePointToSeconds(date));
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToSpecialDay(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска особого дня по дате: " << e.what();
        throw;
    }
}

int64_t SqliteSpecialDayRepository::create(const dto::SpecialDay& specialDay)
{
    if (!specialDay.date.has_value())
    {
        LOG_WARN << "create: отсутствует дата";
        return 0;
    }

    // Проверяем уникальность даты
    if (findByDate(*specialDay.date).has_value())
    {
        LOG_WARN << "create: особый день для даты уже существует";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO SpecialDay (date, isWorkDay, beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (:date, :isWorkDay, :beginWorkTime, :endWorkTime, :breakDuration)"
        );

        stmt->bindInt64("date", common::timePointToSeconds(*specialDay.date));
        stmt->bindInt64("isWorkDay", specialDay.isWorkDay.value_or(false) ? 1 : 0);

        if (specialDay.beginWorkTime.has_value())
            stmt->bindString("beginWorkTime", *specialDay.beginWorkTime);
        else
            stmt->bindNull("beginWorkTime");

        if (specialDay.endWorkTime.has_value())
            stmt->bindString("endWorkTime", *specialDay.endWorkTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", specialDay.breakDuration.value_or(0));

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания особого дня: " << e.what();
        throw;
    }
}

bool SqliteSpecialDayRepository::update(const dto::SpecialDay& specialDay)
{
    if (!specialDay.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID особого дня";
        return false;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "UPDATE SpecialDay SET "
            "isWorkDay = :isWorkDay, "
            "beginWorkTime = :beginWorkTime, "
            "endWorkTime = :endWorkTime, "
            "breakDuration = :breakDuration "
            "WHERE id = :id"
        );

        stmt->bindInt64("id", *specialDay.id);
        stmt->bindInt64("isWorkDay", specialDay.isWorkDay.value_or(false) ? 1 : 0);

        if (specialDay.beginWorkTime.has_value())
            stmt->bindString("beginWorkTime", *specialDay.beginWorkTime);
        else
            stmt->bindNull("beginWorkTime");

        if (specialDay.endWorkTime.has_value())
            stmt->bindString("endWorkTime", *specialDay.endWorkTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", specialDay.breakDuration.value_or(0));

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления особого дня: " << e.what();
        return false;
    }
}

bool SqliteSpecialDayRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM SpecialDay WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления особого дня: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
