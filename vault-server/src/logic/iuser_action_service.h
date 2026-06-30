#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/user_action.h"
#include "common/types.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с действиями пользователя.
 */
struct UserActionsPage
{
    std::vector<dto::UserAction> actions;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с действием пользователя.
 */
struct UserActionResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с действиями пользователя.
 */
class IUserActionService
{
public:
    virtual ~IUserActionService() = default;

    /**
     * @brief Получает список действий пользователя с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по пользователю (std::nullopt - все)
     * @param dateFrom Фильтр по дате начала (std::nullopt - без ограничения)
     * @param dateTo Фильтр по дате окончания (std::nullopt - без ограничения)
     * @return Страница с действиями
     */
    virtual UserActionsPage getActions(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) = 0;

    /**
     * @brief Получает действие по ID.
     * @param id Идентификатор действия
     * @param userId ID пользователя для проверки прав
     * @return DTO действия или std::nullopt
     */
    virtual std::optional<dto::UserAction> getAction(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новое действие.
     * @param action DTO действия
     * @param userId ID пользователя для проверки прав
     * @return Созданное действие или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserAction> createAction(
        const dto::UserAction& action,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет действие.
     * @param id Идентификатор действия
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual UserActionResult deleteAction(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
