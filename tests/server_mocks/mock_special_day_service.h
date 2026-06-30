#pragma once

#include <optional>
#include <vector>

#include "logic/ispecial_day_service.h"

namespace server
{
namespace tests
{

class MockSpecialDayService : public services::ISpecialDayService
{
public:
    // ===== Вспомогательные методы для создания тестовых данных =====
    static dto::SpecialDay createTestSpecialDay(
        int64_t id,
        const std::string& dateStr,
        bool isWorkDay,
        const std::string& beginWorkTime = "",
        const std::string& endWorkTime = "",
        int breakDuration = 0
    )
    {
        dto::SpecialDay day;
        day.id = id;
        day.date = common::stringToDateTime(dateStr + " 00:00:00");
        day.isWorkDay = isWorkDay;

        if (!beginWorkTime.empty())
            day.beginWorkTime = beginWorkTime;
        if (!endWorkTime.empty())
            day.endWorkTime = endWorkTime;
        if (breakDuration > 0 || !isWorkDay)
            day.breakDuration = breakDuration;

        return day;
    }

    // ===== Методы для настройки поведения =====
    void setGetSpecialDaysResult(const services::SpecialDaysPage& result)
    {
        m_getSpecialDaysResult = result;
    }

    void setGetSpecialDayResult(std::optional<dto::SpecialDay> result)
    {
        m_getSpecialDayResult = result;
    }

    void setCreateSpecialDayResult(std::optional<dto::SpecialDay> result)
    {
        m_createSpecialDayResult = result;
    }

    void setUpdateSpecialDayResult(std::optional<dto::SpecialDay> result)
    {
        m_updateSpecialDayResult = result;
    }

    void setDeleteSpecialDayResult(const services::SpecialDayResult& result)
    {
        m_deleteSpecialDayResult = result;
    }

    // ===== Геттеры для проверки вызовов =====
    int getGetSpecialDaysCallCount() const
    {
        return m_getSpecialDaysCallCount;
    }

    int64_t getLastGetSpecialDaysUserId() const
    {
        return m_lastGetSpecialDaysUserId;
    }

    int getLastGetSpecialDaysPage() const
    {
        return m_lastGetSpecialDaysPage;
    }

    int getLastGetSpecialDaysPageSize() const
    {
        return m_lastGetSpecialDaysPageSize;
    }

    std::optional<int> getLastGetSpecialDaysYear() const
    {
        return m_lastGetSpecialDaysYear;
    }

    std::optional<int> getLastGetSpecialDaysMonth() const
    {
        return m_lastGetSpecialDaysMonth;
    }

    int getGetSpecialDayCallCount() const
    {
        return m_getSpecialDayCallCount;
    }

    int64_t getLastGetSpecialDayId() const
    {
        return m_lastGetSpecialDayId;
    }

    int getCreateSpecialDayCallCount() const
    {
        return m_createSpecialDayCallCount;
    }

    int64_t getLastCreateSpecialDayUserId() const
    {
        return m_lastCreateSpecialDayUserId;
    }

    int getUpdateSpecialDayCallCount() const
    {
        return m_updateSpecialDayCallCount;
    }

    int64_t getLastUpdateSpecialDayId() const
    {
        return m_lastUpdateSpecialDayId;
    }

    int getDeleteSpecialDayCallCount() const
    {
        return m_deleteSpecialDayCallCount;
    }

    int64_t getLastDeletedSpecialDayId() const
    {
        return m_lastDeletedSpecialDayId;
    }

    // ===== Реализация интерфейса =====
    services::SpecialDaysPage getSpecialDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int> year = std::nullopt,
        std::optional<int> month = std::nullopt
    ) override
    {
        m_getSpecialDaysCallCount++;
        m_lastGetSpecialDaysUserId = userId;
        m_lastGetSpecialDaysPage = page;
        m_lastGetSpecialDaysPageSize = pageSize;
        m_lastGetSpecialDaysYear = year;
        m_lastGetSpecialDaysMonth = month;
        return m_getSpecialDaysResult;
    }

    std::optional<dto::SpecialDay> getSpecialDay(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getSpecialDayCallCount++;
        m_lastGetSpecialDayId = id;
        m_lastGetSpecialDayUserId = userId;
        return m_getSpecialDayResult;
    }

    std::optional<dto::SpecialDay> getSpecialDayByDate(
        const common::DateTime& date,
        int64_t userId
    ) override
    {
        m_getSpecialDayByDateCallCount++;
        m_lastGetSpecialDayByDate = date;
        m_lastGetSpecialDayByDateUserId = userId;
        return m_getSpecialDayResult;
    }

    std::optional<dto::SpecialDay> createSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) override
    {
        m_createSpecialDayCallCount++;
        m_lastCreateSpecialDayUserId = userId;
        return m_createSpecialDayResult;
    }

    std::optional<dto::SpecialDay> updateSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) override
    {
        m_updateSpecialDayCallCount++;
        if (specialDay.id.has_value())
        {
            m_lastUpdateSpecialDayId = *specialDay.id;
        }
        m_lastUpdateSpecialDayUserId = userId;
        return m_updateSpecialDayResult;
    }

    services::SpecialDayResult deleteSpecialDay(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteSpecialDayCallCount++;
        m_lastDeletedSpecialDayId = id;
        m_lastDeleteSpecialDayUserId = userId;
        return m_deleteSpecialDayResult;
    }

private:
    // Результаты
    services::SpecialDaysPage m_getSpecialDaysResult;
    std::optional<dto::SpecialDay> m_getSpecialDayResult;
    std::optional<dto::SpecialDay> m_createSpecialDayResult;
    std::optional<dto::SpecialDay> m_updateSpecialDayResult;
    services::SpecialDayResult m_deleteSpecialDayResult;

    // Счётчики
    int m_getSpecialDaysCallCount = 0;
    int m_getSpecialDayCallCount = 0;
    int m_getSpecialDayByDateCallCount = 0;
    int m_createSpecialDayCallCount = 0;
    int m_updateSpecialDayCallCount = 0;
    int m_deleteSpecialDayCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetSpecialDaysUserId = 0;
    int m_lastGetSpecialDaysPage = 0;
    int m_lastGetSpecialDaysPageSize = 0;
    std::optional<int> m_lastGetSpecialDaysYear;
    std::optional<int> m_lastGetSpecialDaysMonth;

    int64_t m_lastGetSpecialDayId = 0;
    int64_t m_lastGetSpecialDayUserId = 0;
    common::DateTime m_lastGetSpecialDayByDate;
    int64_t m_lastGetSpecialDayByDateUserId = 0;

    int64_t m_lastCreateSpecialDayUserId = 0;
    int64_t m_lastUpdateSpecialDayId = 0;
    int64_t m_lastUpdateSpecialDayUserId = 0;
    int64_t m_lastDeletedSpecialDayId = 0;
    int64_t m_lastDeleteSpecialDayUserId = 0;
};

} // namespace tests
} // namespace server
