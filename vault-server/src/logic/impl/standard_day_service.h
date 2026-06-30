#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/istandard_day_service.h"

#include "repo/standard_day_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы со стандартными днями.
 */
class StandardDayService final : public IStandardDayService
{
public:
    /**
     * @brief Конструктор.
     * @param standardDayRepo Репозиторий стандартных дней
     * @param authzService Сервис авторизации для проверки прав
     */
    StandardDayService(
        std::shared_ptr<repositories::IStandardDayRepository> standardDayRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IStandardDayService
    std::vector<dto::StandardDay> getAllStandardDays(int64_t userId) override;
    std::optional<dto::StandardDay> getStandardDayByWeekDay(
        int weekDayNumber,
        int64_t userId
    ) override;
    StandardDayResult updateStandardDay(
        const dto::StandardDay& standardDay,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет, имеет ли пользователь право на изменение календаря.
     * @param userId ID пользователя
     * @return true если пользователь имеет право
     */
    bool canModifyCalendar(int64_t userId);

    /**
     * @brief Валидирует DTO стандартного дня.
     * @param standardDay DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateStandardDay(
        const dto::StandardDay& standardDay,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IStandardDayRepository> m_standardDayRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
