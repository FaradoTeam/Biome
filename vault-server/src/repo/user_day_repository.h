#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_day.h"
#include "common/types.h"

namespace server
{
namespace repositories
{

/**
 * @brief Структура для возврата пагинированного списка пользовательских дней.
 */
struct UserDaysPage
{
    std::vector<dto::UserDay> days;
    int64_t totalCount = 0;
};

/**
 * @brief Абстрактный интерфейс репозитория для работы с пользовательскими днями (UserDay).
 */
class IUserDayRepository
{
public:
    virtual ~IUserDayRepository() = default;

    /**
     * @brief Получает список пользовательских дней с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1).
     * @param pageSize Количество записей на странице.
     * @param userId Фильтр по пользователю (std::nullopt - все).
     * @param dateFrom Фильтр по дате начала (std::nullopt - без ограничения).
     * @param dateTo Фильтр по дате окончания (std::nullopt - без ограничения).
     * @return Страница с пользовательскими днями.
     */
    virtual UserDaysPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Находит пользовательский день по ID.
     * @param id Идентификатор записи.
     * @return DTO пользовательского дня или std::nullopt.
     */
    virtual std::optional<dto::UserDay> findById(int64_t id) = 0;

    /**
     * @brief Находит пользовательский день по пользователю и дате.
     * @param userId Идентификатор пользователя.
     * @param date Дата для поиска.
     * @return DTO пользовательского дня или std::nullopt.
     */
    virtual std::optional<dto::UserDay> findByUserAndDate(int64_t userId, const common::DateTime& date) = 0;

    /**
     * @brief Находит все пользовательские дни для указанного пользователя.
     * @param userId Идентификатор пользователя.
     * @return Вектор пользовательских дней.
     */
    virtual std::vector<dto::UserDay> findByUserId(int64_t userId) = 0;

    /**
     * @brief Создаёт новый пользовательский день.
     * @param userDay DTO пользовательского дня.
     * @return ID созданной записи или 0 при ошибке.
     */
    virtual int64_t create(const dto::UserDay& userDay) = 0;

    /**
     * @brief Обновляет существующий пользовательский день.
     * @param userDay DTO с новыми данными. Поле id обязательно.
     * @return true если обновление успешно.
     */
    virtual bool update(const dto::UserDay& userDay) = 0;

    /**
     * @brief Удаляет пользовательский день по ID.
     * @param id Идентификатор записи.
     * @return true если удаление успешно.
     */
    virtual bool remove(int64_t id) = 0;

    /**
     * @brief Удаляет все пользовательские дни для указанного пользователя.
     * @param userId Идентификатор пользователя.
     * @return Количество удалённых записей.
     */
    virtual int64_t removeByUserId(int64_t userId) = 0;
};

} // namespace repositories
} // namespace server
