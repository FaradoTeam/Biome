#pragma once

#include <optional>
#include <vector>

#include "common/dto/special_day.h"
#include "common/types.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с особыми днями.
 */
struct SpecialDaysPage
{
    std::vector<dto::SpecialDay> days;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с особым днём.
 */
struct SpecialDayResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с особыми днями.
 */
class ISpecialDayService
{
public:
    virtual ~ISpecialDayService() = default;

    /**
     * @brief Получает список особых дней с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param year Фильтр по году (std::nullopt - все)
     * @param month Фильтр по месяцу (std::nullopt - все)
     * @return Страница с особыми днями
     */
    virtual SpecialDaysPage getSpecialDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int> year = std::nullopt,
        std::optional<int> month = std::nullopt
    ) = 0;

    /**
     * @brief Получает особый день по ID.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return DTO особого дня или std::nullopt
     */
    virtual std::optional<dto::SpecialDay> getSpecialDay(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает особый день по дате.
     * @param date Дата
     * @param userId ID пользователя для проверки прав
     * @return DTO особого дня или std::nullopt
     */
    virtual std::optional<dto::SpecialDay> getSpecialDayByDate(
        const common::DateTime& date,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новый особый день.
     * @param specialDay DTO особого дня
     * @param userId ID пользователя для проверки прав
     * @return Созданный DTO или std::nullopt при ошибке
     */
    virtual std::optional<dto::SpecialDay> createSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет особый день.
     * @param specialDay DTO особого дня с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённый DTO или std::nullopt при ошибке
     */
    virtual std::optional<dto::SpecialDay> updateSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет особый день.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual SpecialDayResult deleteSpecialDay(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
