#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/role.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка ролей.
 */
struct RolesPage
{
    std::vector<dto::Role> roles;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления ролями.
 */
class IRoleService
{
public:
    virtual ~IRoleService() = default;

    /**
     * @brief Получает список ролей с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param searchCaption Поиск по названию (опционально)
     * @return Страница с ролями
     */
    virtual RolesPage getRoles(
        int page,
        int pageSize,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает роль по ID.
     * @param id Идентификатор роли
     * @return DTO роли или std::nullopt
     */
    virtual std::optional<dto::Role> getRole(int64_t id) = 0;

    /**
     * @brief Создает новую роль.
     * @param role DTO роли
     * @param userId ID пользователя для проверки прав
     * @return Созданная роль или std::nullopt при ошибке
     */
    virtual std::optional<dto::Role> createRole(
        const dto::Role& role,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую роль.
     * @param role DTO роли с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленная роль или std::nullopt при ошибке
     */
    virtual std::optional<dto::Role> updateRole(
        const dto::Role& role,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет роль.
     * @param id Идентификатор роли
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRole(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
