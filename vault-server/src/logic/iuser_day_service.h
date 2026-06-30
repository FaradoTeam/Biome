#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_day.h"
#include "common/types.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с пользовательскими днями.
 */
struct UserDaysPage
{
    std::vector<dto::UserDay> days;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с пользовательским днём.
 */
struct UserDayResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с пользовательскими днями.
 */
class IUserDayService
{
public:
    virtual ~IUserDayService() = default;

    /**
     * @brief Получает список пользовательских дней с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по пользователю (std::nullopt - все)
     * @param dateFrom Фильтр по дате начала (std::nullopt - без ограничения)
     * @param dateTo Фильтр по дате окончания (std::nullopt - без ограничения)
     * @return Страница с пользовательскими днями
     */
    virtual UserDaysPage getUserDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Получает пользовательский день по ID.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return DTO пользовательского дня или std::nullopt
     */
    virtual std::optional<dto::UserDay> getUserDay(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает пользовательский день по пользователю и дате.
     * @param userId ID пользователя
     * @param date Дата
     * @param currentUserId ID текущего пользователя для проверки прав
     * @return DTO пользовательского дня или std::nullopt
     */
    virtual std::optional<dto::UserDay> getUserDayByUserAndDate(
        int64_t userId,
        const common::DateTime& date,
        int64_t currentUserId
    ) = 0;

    /**
     * @brief Создаёт новый пользовательский день.
     * @param userDay DTO пользовательского дня
     * @param currentUserId ID текущего пользователя для проверки прав
     * @return Созданный DTO или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserDay> createUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) = 0;

    /**
     * @brief Обновляет пользовательский день.
     * @param userDay DTO пользовательского дня с новыми данными
     * @param currentUserId ID текущего пользователя для проверки прав
     * @return Обновлённый DTO или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserDay> updateUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) = 0;

    /**
     * @brief Удаляет пользовательский день.
     * @param id Идентификатор записи
     * @param currentUserId ID текущего пользователя для проверки прав
     * @return Результат операции
     */
    virtual UserDayResult deleteUserDay(
        int64_t id,
        int64_t currentUserId
    ) = 0;

    /**
     * @brief Удаляет все пользовательские дни для указанного пользователя.
     * @param userId ID пользователя
     * @param currentUserId ID текущего пользователя для проверки прав
     * @return Количество удалённых записей
     */
    virtual int64_t deleteUserDaysByUser(
        int64_t userId,
        int64_t currentUserId
    ) = 0;
};

} // namespace services
} // namespace server
