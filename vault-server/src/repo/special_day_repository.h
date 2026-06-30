#pragma once

#include <optional>
#include <vector>

#include "common/dto/special_day.h"
#include "common/types.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка особых дней.
 */
struct SpecialDaysPage
{
    std::vector<dto::SpecialDay> days;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с особыми днями (SpecialDay).
 */
class ISpecialDayRepository
{
public:
    virtual ~ISpecialDayRepository() = default;

    /**
     * @brief Получает список особых дней с пагинацией и фильтрацией по году/месяцу.
     * @param page Номер страницы (начиная с 1).
     * @param pageSize Количество записей на странице.
     * @param year Фильтр по году (std::nullopt - все).
     * @param month Фильтр по месяцу (std::nullopt - все).
     * @return Страница с особыми днями.
     */
    virtual SpecialDaysPage findAll(
        int page,
        int pageSize,
        std::optional<int> year = std::nullopt,
        std::optional<int> month = std::nullopt
    ) = 0;

    /**
     * @brief Находит особый день по ID.
     * @param id Идентификатор записи.
     * @return DTO особого дня или std::nullopt.
     */
    virtual std::optional<dto::SpecialDay> findById(int64_t id) = 0;

    /**
     * @brief Находит особый день по дате.
     * @param date Дата для поиска.
     * @return DTO особого дня или std::nullopt.
     */
    virtual std::optional<dto::SpecialDay> findByDate(const common::DateTime& date) = 0;

    /**
     * @brief Создаёт новый особый день.
     * @param specialDay DTO особого дня.
     * @return ID созданной записи или 0 при ошибке.
     */
    virtual int64_t create(const dto::SpecialDay& specialDay) = 0;

    /**
     * @brief Обновляет существующий особый день.
     * @param specialDay DTO с новыми данными. Поле id обязательно.
     * @return true если обновление успешно.
     */
    virtual bool update(const dto::SpecialDay& specialDay) = 0;

    /**
     * @brief Удаляет особый день по ID.
     * @param id Идентификатор записи.
     * @return true если удаление успешно.
     */
    virtual bool remove(int64_t id) = 0;
};

} // namespace repositories
} // namespace server
