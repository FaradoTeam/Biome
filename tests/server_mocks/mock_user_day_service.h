#pragma once

#include <optional>
#include <vector>

#include "logic/iuser_day_service.h"

namespace server
{
namespace tests
{

class MockUserDayService : public services::IUserDayService
{
public:
    // ===== Вспомогательные методы для создания тестовых данных =====
    static dto::UserDay createTestUserDay(
        int64_t id,
        int64_t userId,
        const std::string& dateStr,
        bool isWorkDay,
        const std::string& beginWorkTime,
        const std::string& endWorkTime,
        int breakDuration,
        const std::string& description
    )
    {
        dto::UserDay day;
        day.id = id;
        day.userId = userId;
        day.date = common::stringToDateTime(dateStr + " 00:00:00");
        day.isWorkDay = isWorkDay;

        if (!beginWorkTime.empty())
            day.beginWorkTime = beginWorkTime;
        if (!endWorkTime.empty())
            day.endWorkTime = endWorkTime;
        if (breakDuration > 0)
            day.breakDuration = breakDuration;
        if (!description.empty())
            day.description = description;

        return day;
    }

    // ===== Методы для настройки поведения =====
    void setGetUserDaysResult(const services::UserDaysPage& result)
    {
        m_getUserDaysResult = result;
    }

    void setGetUserDayResult(std::optional<dto::UserDay> result)
    {
        m_getUserDayResult = result;
    }

    void setGetUserDayByUserAndDateResult(std::optional<dto::UserDay> result)
    {
        m_getUserDayByUserAndDateResult = result;
    }

    void setCreateUserDayResult(std::optional<dto::UserDay> result)
    {
        m_createUserDayResult = result;
    }

    void setUpdateUserDayResult(std::optional<dto::UserDay> result)
    {
        m_updateUserDayResult = result;
    }

    void setDeleteUserDayResult(const services::UserDayResult& result)
    {
        m_deleteUserDayResult = result;
    }

    void setDeleteUserDaysByUserResult(int64_t result)
    {
        m_deleteUserDaysByUserResult = result;
    }

    // ===== Геттеры для проверки вызовов =====
    int getGetUserDaysCallCount() const
    {
        return m_getUserDaysCallCount;
    }

    int64_t getLastGetUserDaysUserId() const
    {
        return m_lastGetUserDaysUserId;
    }

    std::optional<int64_t> getLastGetUserDaysFilterUserId() const
    {
        return m_lastGetUserDaysFilterUserId;
    }

    std::optional<common::DateTime> getLastGetUserDaysDateFrom() const
    {
        return m_lastGetUserDaysDateFrom;
    }

    std::optional<common::DateTime> getLastGetUserDaysDateTo() const
    {
        return m_lastGetUserDaysDateTo;
    }

    int getGetUserDayCallCount() const
    {
        return m_getUserDayCallCount;
    }

    int64_t getLastGetUserDayId() const
    {
        return m_lastGetUserDayId;
    }

    int getGetUserDayByUserAndDateCallCount() const
    {
        return m_getUserDayByUserAndDateCallCount;
    }

    int64_t getLastGetUserDayByUserAndDateUserId() const
    {
        return m_lastGetUserDayByUserAndDateUserId;
    }

    int getCreateUserDayCallCount() const
    {
        return m_createUserDayCallCount;
    }

    int64_t getLastCreateUserDayUserId() const
    {
        return m_lastCreateUserDayCurrentUserId;
    }

    int getUpdateUserDayCallCount() const
    {
        return m_updateUserDayCallCount;
    }

    int64_t getLastUpdateUserDayId() const
    {
        return m_lastUpdateUserDayId;
    }

    int getDeleteUserDayCallCount() const
    {
        return m_deleteUserDayCallCount;
    }

    int64_t getLastDeletedUserDayId() const
    {
        return m_lastDeletedUserDayId;
    }

    int getDeleteUserDaysByUserCallCount() const
    {
        return m_deleteUserDaysByUserCallCount;
    }

    int64_t getLastDeleteUserDaysByUserUserId() const
    {
        return m_lastDeleteUserDaysByUserUserId;
    }

    // ===== Реализация интерфейса =====
    services::UserDaysPage getUserDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override
    {
        m_getUserDaysCallCount++;
        m_lastGetUserDaysUserId = userId;
        m_lastGetUserDaysFilterUserId = filterUserId;
        m_lastGetUserDaysDateFrom = dateFrom;
        m_lastGetUserDaysDateTo = dateTo;
        m_lastGetUserDaysPage = page;
        m_lastGetUserDaysPageSize = pageSize;
        return m_getUserDaysResult;
    }

    std::optional<dto::UserDay> getUserDay(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getUserDayCallCount++;
        m_lastGetUserDayId = id;
        m_lastGetUserDayUserId = userId;
        return m_getUserDayResult;
    }

    std::optional<dto::UserDay> getUserDayByUserAndDate(
        int64_t userId,
        const common::DateTime& date,
        int64_t currentUserId
    ) override
    {
        m_getUserDayByUserAndDateCallCount++;
        m_lastGetUserDayByUserAndDateUserId = userId;
        m_lastGetUserDayByUserAndDateDate = date;
        m_lastGetUserDayByUserAndDateCurrentUserId = currentUserId;
        return m_getUserDayByUserAndDateResult;
    }

    std::optional<dto::UserDay> createUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) override
    {
        m_createUserDayCallCount++;
        m_lastCreateUserDayCurrentUserId = currentUserId;
        return m_createUserDayResult;
    }

    std::optional<dto::UserDay> updateUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) override
    {
        m_updateUserDayCallCount++;
        if (userDay.id.has_value())
        {
            m_lastUpdateUserDayId = *userDay.id;
        }
        m_lastUpdateUserDayCurrentUserId = currentUserId;
        return m_updateUserDayResult;
    }

    services::UserDayResult deleteUserDay(
        int64_t id,
        int64_t currentUserId
    ) override
    {
        m_deleteUserDayCallCount++;
        m_lastDeletedUserDayId = id;
        m_lastDeleteUserDayCurrentUserId = currentUserId;
        return m_deleteUserDayResult;
    }

    int64_t deleteUserDaysByUser(
        int64_t userId,
        int64_t currentUserId
    ) override
    {
        m_deleteUserDaysByUserCallCount++;
        m_lastDeleteUserDaysByUserUserId = userId;
        m_lastDeleteUserDaysByUserCurrentUserId = currentUserId;
        return m_deleteUserDaysByUserResult;
    }

private:
    // Результаты
    services::UserDaysPage m_getUserDaysResult;
    std::optional<dto::UserDay> m_getUserDayResult;
    std::optional<dto::UserDay> m_getUserDayByUserAndDateResult;
    std::optional<dto::UserDay> m_createUserDayResult;
    std::optional<dto::UserDay> m_updateUserDayResult;
    services::UserDayResult m_deleteUserDayResult;
    int64_t m_deleteUserDaysByUserResult = 0;

    // Счётчики
    int m_getUserDaysCallCount = 0;
    int m_getUserDayCallCount = 0;
    int m_getUserDayByUserAndDateCallCount = 0;
    int m_createUserDayCallCount = 0;
    int m_updateUserDayCallCount = 0;
    int m_deleteUserDayCallCount = 0;
    int m_deleteUserDaysByUserCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetUserDaysUserId = 0;
    std::optional<int64_t> m_lastGetUserDaysFilterUserId;
    std::optional<common::DateTime> m_lastGetUserDaysDateFrom;
    std::optional<common::DateTime> m_lastGetUserDaysDateTo;
    int m_lastGetUserDaysPage = 0;
    int m_lastGetUserDaysPageSize = 0;

    int64_t m_lastGetUserDayId = 0;
    int64_t m_lastGetUserDayUserId = 0;

    int64_t m_lastGetUserDayByUserAndDateUserId = 0;
    common::DateTime m_lastGetUserDayByUserAndDateDate;
    int64_t m_lastGetUserDayByUserAndDateCurrentUserId = 0;

    int64_t m_lastCreateUserDayCurrentUserId = 0;
    int64_t m_lastUpdateUserDayId = 0;
    int64_t m_lastUpdateUserDayCurrentUserId = 0;
    int64_t m_lastDeletedUserDayId = 0;
    int64_t m_lastDeleteUserDayCurrentUserId = 0;
    int64_t m_lastDeleteUserDaysByUserUserId = 0;
    int64_t m_lastDeleteUserDaysByUserCurrentUserId = 0;
};

} // namespace tests
} // namespace server
