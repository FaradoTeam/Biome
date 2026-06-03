#pragma once

#include <optional>
#include <vector>

#include "common/dto/user_team_role.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка назначений пользователей в команды.
 */
struct UserTeamRolesPage
{
    std::vector<dto::UserTeamRole> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления назначениями пользователей в команды.
 */
class IUserTeamRoleService
{
public:
    virtual ~IUserTeamRoleService() = default;

    /**
     * @brief Получает список назначений пользователей в команды с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по идентификатору пользователя (опционально)
     * @param teamId Фильтр по идентификатору команды (опционально)
     * @param roleId Фильтр по идентификатору роли (опционально)
     * @return Страница с назначениями
     */
    virtual UserTeamRolesPage getUserTeamRoles(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    /**
     * @brief Получает назначение по ID.
     * @param id Идентификатор назначения
     * @param userId ID пользователя для проверки прав
     * @return DTO назначения или std::nullopt
     */
    virtual std::optional<dto::UserTeamRole> getUserTeamRole(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создает новое назначение пользователя в команду.
     * @param userTeamRole DTO назначения
     * @param userId ID пользователя для проверки прав
     * @return Созданное назначение или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserTeamRole> createUserTeamRole(
        const dto::UserTeamRole& userTeamRole,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее назначение.
     * @param userTeamRole DTO назначения с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное назначение или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserTeamRole> updateUserTeamRole(
        const dto::UserTeamRole& userTeamRole,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет назначение.
     * @param id Идентификатор назначения
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteUserTeamRole(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
