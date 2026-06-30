#include <stdexcept>

#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_standard_day_repository.h"

namespace server
{
namespace repositories
{

SqliteStandardDayRepository::SqliteStandardDayRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteStandardDayRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteStandardDayRepository::connection() const
{
    return m_database->connection();
}

dto::StandardDay SqliteStandardDayRepository::mapRowToStandardDay(db::IResultSet& rs) const
{
    dto::StandardDay day;
    day.id = rs.valueInt64("id");
    day.weekDayNumber = rs.valueInt64("weekDayNumber");
    day.weekOrder = rs.valueInt64("weekOrder");
    day.isWorkDay = rs.valueInt64("isWorkDay") != 0;

    if (!rs.isNull("beginWorkTime"))
        day.beginWorkTime = rs.valueString("beginWorkTime");

    if (!rs.isNull("endWorkTime"))
        day.endWorkTime = rs.valueString("endWorkTime");

    if (!rs.isNull("breakDuration"))
        day.breakDuration = rs.valueInt64("breakDuration");

    return day;
}

std::vector<dto::StandardDay> SqliteStandardDayRepository::findAll()
{
    std::vector<dto::StandardDay> days;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration "
            "FROM StandardDay ORDER BY weekDayNumber"
        );

        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            days.push_back(mapRowToStandardDay(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка стандартных дней: " << e.what();
        throw;
    }

    return days;
}

std::optional<dto::StandardDay> SqliteStandardDayRepository::findByWeekDayNumber(
    int weekDayNumber
)
{
    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        LOG_WARN << "findByWeekDayNumber: неверный номер дня недели " << weekDayNumber;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration "
            "FROM StandardDay WHERE weekDayNumber = :weekDayNumber"
        );

        stmt->bindInt64("weekDayNumber", weekDayNumber);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToStandardDay(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска стандартного дня по номеру: " << e.what();
        throw;
    }
}

bool SqliteStandardDayRepository::update(const dto::StandardDay& standardDay)
{
    if (!standardDay.weekDayNumber.has_value())
    {
        LOG_WARN << "update: отсутствует номер дня недели";
        return false;
    }

    try
    {
        auto conn = connection();

        // Сначала проверяем, существует ли запись
        auto existing = findByWeekDayNumber(*standardDay.weekDayNumber);
        if (!existing.has_value())
        {
            LOG_WARN << "update: запись для дня " << *standardDay.weekDayNumber << " не найдена";
            return false;
        }

        // Используем существующие значения, если новые не переданы
        int weekOrder = standardDay.weekOrder.has_value() ? *standardDay.weekOrder : *existing->weekOrder;
        bool isWorkDay = standardDay.isWorkDay.has_value() ? *standardDay.isWorkDay : *existing->isWorkDay;

        // Для времени работы:
        // - Если isWorkDay = false, время должно быть NULL
        // - Если isWorkDay = true, используем переданное или существующее
        std::optional<std::string> beginWorkTime;
        std::optional<std::string> endWorkTime;

        if (!isWorkDay)
        {
            // Выходной день - время NULL
            beginWorkTime = std::nullopt;
            endWorkTime = std::nullopt;
        }
        else
        {
            // Рабочий день - используем переданное или существующее
            beginWorkTime = standardDay.beginWorkTime.has_value() ? standardDay.beginWorkTime : existing->beginWorkTime;
            endWorkTime = standardDay.endWorkTime.has_value() ? standardDay.endWorkTime : existing->endWorkTime;
        }

        int breakDuration = standardDay.breakDuration.has_value() ? *standardDay.breakDuration : *existing->breakDuration;
        // Если выходной, перерыв тоже 0
        if (!isWorkDay)
        {
            breakDuration = 0;
        }

        auto stmt = conn->prepareStatement(
            "UPDATE StandardDay SET "
            "weekOrder = :weekOrder, "
            "isWorkDay = :isWorkDay, "
            "beginWorkTime = :beginWorkTime, "
            "endWorkTime = :endWorkTime, "
            "breakDuration = :breakDuration "
            "WHERE weekDayNumber = :weekDayNumber"
        );

        stmt->bindInt64("weekDayNumber", *standardDay.weekDayNumber);
        stmt->bindInt64("weekOrder", weekOrder);
        stmt->bindInt64("isWorkDay", isWorkDay ? 1 : 0);

        if (beginWorkTime.has_value())
            stmt->bindString("beginWorkTime", *beginWorkTime);
        else
            stmt->bindNull("beginWorkTime");

        if (endWorkTime.has_value())
            stmt->bindString("endWorkTime", *endWorkTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", breakDuration);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления стандартного дня: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
