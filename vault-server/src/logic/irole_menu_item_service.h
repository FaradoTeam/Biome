#pragma once

#include <optional>
#include <vector>

#include "common/dto/role_menu_item.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка пунктов меню.
 */
struct RoleMenuItemsPage
{
    std::vector<dto::RoleMenuItem> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления пунктами меню ролей.
 */
class IRoleMenuItemService
{
public:
    virtual ~IRoleMenuItemService() = default;

    /**
     * @brief Получает список пунктов меню с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param roleId Фильтр по идентификатору роли (опционально)
     * @return Страница с пунктами меню
     */
    virtual RoleMenuItemsPage getRoleMenuItems(
        int page,
        int pageSize,
        std::optional<int64_t> roleId = std::nullopt
    ) = 0;

    /**
     * @brief Получает пункт меню по ID.
     * @param id Идентификатор пункта меню
     * @return DTO пункта меню или std::nullopt
     */
    virtual std::optional<dto::RoleMenuItem> getRoleMenuItem(int64_t id) = 0;

    /**
     * @brief Создает новый пункт меню для роли.
     * @param item DTO пункта меню
     * @param userId ID пользователя для проверки прав
     * @return Созданный пункт меню или std::nullopt при ошибке
     */
    virtual std::optional<dto::RoleMenuItem> createRoleMenuItem(
        const dto::RoleMenuItem& item,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий пункт меню.
     * @param item DTO пункта меню с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленный пункт меню или std::nullopt при ошибке
     */
    virtual std::optional<dto::RoleMenuItem> updateRoleMenuItem(
        const dto::RoleMenuItem& item,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет пункт меню.
     * @param id Идентификатор пункта меню
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteRoleMenuItem(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
