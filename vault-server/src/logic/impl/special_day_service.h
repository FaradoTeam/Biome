#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/ispecial_day_service.h"

#include "repo/special_day_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с особыми днями.
 */
class SpecialDayService final : public ISpecialDayService
{
public:
    /**
     * @brief Конструктор.
     * @param specialDayRepo Репозиторий особых дней
     * @param authzService Сервис авторизации для проверки прав
     */
    SpecialDayService(
        std::shared_ptr<repositories::ISpecialDayRepository> specialDayRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // ISpecialDayService
    SpecialDaysPage getSpecialDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int> year = std::nullopt,
        std::optional<int> month = std::nullopt
    ) override;

    std::optional<dto::SpecialDay> getSpecialDay(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::SpecialDay> getSpecialDayByDate(
        const common::DateTime& date,
        int64_t userId
    ) override;

    std::optional<dto::SpecialDay> createSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) override;

    std::optional<dto::SpecialDay> updateSpecialDay(
        const dto::SpecialDay& specialDay,
        int64_t userId
    ) override;

    SpecialDayResult deleteSpecialDay(
        int64_t id,
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
     * @brief Валидирует DTO особого дня.
     * @param specialDay DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateSpecialDay(
        const dto::SpecialDay& specialDay,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::ISpecialDayRepository> m_specialDayRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
