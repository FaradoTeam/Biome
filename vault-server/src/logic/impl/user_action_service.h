#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_action_service.h"

#include "repo/user_action_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с действиями пользователя.
 */
class UserActionService final : public IUserActionService
{
public:
    /**
     * @brief Конструктор.
     * @param actionRepo Репозиторий действий пользователя
     * @param authzService Сервис авторизации для проверки прав
     */
    UserActionService(
        std::shared_ptr<repositories::IUserActionRepository> actionRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserActionService
    UserActionsPage getActions(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override;

    std::optional<dto::UserAction> getAction(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::UserAction> createAction(
        const dto::UserAction& action,
        int64_t userId
    ) override;

    UserActionResult deleteAction(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к действию.
     * @param actionId ID действия
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO действия или std::nullopt
     */
    std::optional<dto::UserAction> checkActionAccess(
        int64_t actionId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Валидирует DTO действия.
     * @param action DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateAction(
        const dto::UserAction& action,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IUserActionRepository> m_actionRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
