#pragma once

#include <optional>
#include <vector>

#include "common/dto/standard_day.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции со стандартным днём.
 */
struct StandardDayResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы со стандартными днями.
 */
class IStandardDayService
{
public:
    virtual ~IStandardDayService() = default;

    /**
     * @brief Получает список всех стандартных дней.
     * @param userId ID пользователя для проверки прав
     * @return Вектор DTO стандартных дней
     */
    virtual std::vector<dto::StandardDay> getAllStandardDays(int64_t userId) = 0;

    /**
     * @brief Получает стандартный день по номеру дня недели.
     * @param weekDayNumber Номер дня недели (0-6, где 0 - воскресенье)
     * @param userId ID пользователя для проверки прав
     * @return DTO стандартного дня или std::nullopt
     */
    virtual std::optional<dto::StandardDay> getStandardDayByWeekDay(
        int weekDayNumber,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет стандартный день.
     * @param standardDay DTO стандартного дня с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual StandardDayResult updateStandardDay(
        const dto::StandardDay& standardDay,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
