#pragma once

#include <optional>
#include <vector>

#include "logic/istandard_day_service.h"

namespace server
{
namespace tests
{

class MockStandardDayService : public services::IStandardDayService
{
public:
    // ===== Методы для настройки поведения =====
    void setGetAllStandardDaysResult(const std::vector<dto::StandardDay>& result)
    {
        m_getAllStandardDaysResult = result;
    }

    void setGetStandardDayByWeekDayResult(std::optional<dto::StandardDay> result)
    {
        m_getStandardDayByWeekDayResult = result;
    }

    void setUpdateStandardDayResult(const services::StandardDayResult& result)
    {
        m_updateStandardDayResult = result;
    }

    // ===== Геттеры для проверки вызовов =====
    int getGetAllStandardDaysCallCount() const
    {
        return m_getAllStandardDaysCallCount;
    }

    int getGetStandardDayByWeekDayCallCount() const
    {
        return m_getStandardDayByWeekDayCallCount;
    }

    int getLastWeekDayNumber() const
    {
        return m_lastWeekDayNumber;
    }

    int getUpdateStandardDayCallCount() const
    {
        return m_updateStandardDayCallCount;
    }

    int getLastUpdateWeekDayNumber() const
    {
        return m_lastUpdateWeekDayNumber;
    }

    int64_t getLastUpdateUserId() const
    {
        return m_lastUpdateUserId;
    }

    // ===== Реализация интерфейса =====
    std::vector<dto::StandardDay> getAllStandardDays(int64_t userId) override
    {
        m_getAllStandardDaysCallCount++;
        m_lastGetAllStandardDaysUserId = userId;
        return m_getAllStandardDaysResult;
    }

    std::optional<dto::StandardDay> getStandardDayByWeekDay(
        int weekDayNumber,
        int64_t userId
    ) override
    {
        m_getStandardDayByWeekDayCallCount++;
        m_lastWeekDayNumber = weekDayNumber;
        m_lastGetStandardDayByWeekDayUserId = userId;
        return m_getStandardDayByWeekDayResult;
    }

    services::StandardDayResult updateStandardDay(
        const dto::StandardDay& standardDay,
        int64_t userId
    ) override
    {
        m_updateStandardDayCallCount++;
        if (standardDay.weekDayNumber.has_value())
        {
            m_lastUpdateWeekDayNumber = *standardDay.weekDayNumber;
        }
        m_lastUpdateUserId = userId;
        return m_updateStandardDayResult;
    }

private:
    // Результаты
    std::vector<dto::StandardDay> m_getAllStandardDaysResult;
    std::optional<dto::StandardDay> m_getStandardDayByWeekDayResult;
    services::StandardDayResult m_updateStandardDayResult;

    // Счётчики
    int m_getAllStandardDaysCallCount = 0;
    int m_getStandardDayByWeekDayCallCount = 0;
    int m_updateStandardDayCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetAllStandardDaysUserId = 0;
    int64_t m_lastGetStandardDayByWeekDayUserId = 0;
    int m_lastWeekDayNumber = -1;
    int m_lastUpdateWeekDayNumber = -1;
    int64_t m_lastUpdateUserId = 0;
};

} // namespace tests
} // namespace server
