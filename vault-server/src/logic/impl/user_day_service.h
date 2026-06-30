#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_day_service.h"

#include "repo/user_day_repository.h"
#include "repo/user_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с пользовательскими днями.
 */
class UserDayService final : public IUserDayService
{
public:
    /**
     * @brief Конструктор.
     * @param userDayRepo Репозиторий пользовательских дней
     * @param userRepo Репозиторий пользователей
     * @param authzService Сервис авторизации для проверки прав
     */
    UserDayService(
        std::shared_ptr<repositories::IUserDayRepository> userDayRepo,
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserDayService
    UserDaysPage getUserDays(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::UserDay> getUserDay(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::UserDay> getUserDayByUserAndDate(
        int64_t userId,
        const common::DateTime& date,
        int64_t currentUserId
    ) override;

    std::optional<dto::UserDay> createUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) override;

    std::optional<dto::UserDay> updateUserDay(
        const dto::UserDay& userDay,
        int64_t currentUserId
    ) override;

    UserDayResult deleteUserDay(
        int64_t id,
        int64_t currentUserId
    ) override;

    int64_t deleteUserDaysByUser(
        int64_t userId,
        int64_t currentUserId
    ) override;

private:
    /**
     * @brief Проверяет, может ли пользователь управлять днём другого пользователя.
     * @param targetUserId ID целевого пользователя
     * @param currentUserId ID текущего пользователя
     * @return true если разрешено
     */
    bool canManageUserDay(int64_t targetUserId, int64_t currentUserId);

    /**
     * @brief Проверяет существование пользователя.
     * @param userId ID пользователя
     * @return true если пользователь существует
     */
    bool userExists(int64_t userId);

    /**
     * @brief Валидирует DTO пользовательского дня.
     * @param userDay DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateUserDay(
        const dto::UserDay& userDay,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IUserDayRepository> m_userDayRepo;
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
