#pragma once

#include <optional>
#include <vector>

#include "common/dto/standard_day.h"

namespace server
{
namespace repositories
{

/**
 * @brief Абстрактный интерфейс репозитория для работы со стандартными днями (StandardDay).
 */
class IStandardDayRepository
{
public:
    virtual ~IStandardDayRepository() = default;

    /**
     * @brief Получает список всех стандартных дней.
     * @return Вектор DTO стандартных дней.
     */
    virtual std::vector<dto::StandardDay> findAll() = 0;

    /**
     * @brief Находит стандартный день по номеру дня недели.
     * @param weekDayNumber Номер дня недели (0-6, где 0 - воскресенье).
     * @return DTO стандартного дня или std::nullopt, если не найден.
     */
    virtual std::optional<dto::StandardDay> findByWeekDayNumber(int weekDayNumber) = 0;

    /**
     * @brief Обновляет существующий стандартный день.
     * @param standardDay DTO с новыми данными. Поле weekDayNumber обязательно.
     * @return true если обновление успешно.
     */
    virtual bool update(const dto::StandardDay& standardDay) = 0;
};

} // namespace repositories
} // namespace server
