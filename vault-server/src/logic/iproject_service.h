#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/project.h"

namespace server
{
namespace services
{

/**
 * @brief Структура для возврата результатов пагинированного списка проектов.
 */
struct ProjectsPage
{
    std::vector<dto::Project> projects;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления проектами.
 */
class IProjectService
{
public:
    virtual ~IProjectService() = default;

    /**
     * @brief Получает список проектов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param parentId Фильтр по родительскому проекту (std::nullopt - все)
     * @param isArchive Фильтр по статусу архивации (std::nullopt - все)
     * @param searchCaption Поиск по названию (пустая строка - без поиска)
     * @return Объект ProjectsPage, содержащий список проектов и общее количество.
     */
    virtual ProjectsPage projects(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<bool> isArchive = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает проект по его ID.
     * @param id Идентификатор проекта.
     * @param userId ID пользователя для проверки прав
     * @return DTO проекта или std::nullopt, если не найден или нет прав.
     */
    virtual std::optional<dto::Project> project(int64_t id, int64_t userId) = 0;

    /**
     * @brief Создает новый проект.
     * @param project DTO проекта (поля createdAt/updatedAt будут проигнорированы).
     * @param userId ID пользователя, создающего проект
     * @return DTO созданного проекта с присвоенным ID или std::nullopt при ошибке.
     */
    virtual std::optional<dto::Project> createProject(
        const dto::Project& project,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий проект.
     * @param project DTO проекта с новыми данными. Поле id обязательно.
     * @param userId ID пользователя, обновляющего проект
     * @return DTO обновленного проекта или std::nullopt, если проект не найден
     * или обновление не удалось или нет прав.
     */
    virtual std::optional<dto::Project> updateProject(
        const dto::Project& project,
        int64_t userId
    ) = 0;

    /**
     * @brief Архивирует (мягкое удаление) проект.
     * @param id Идентификатор проекта.
     * @param userId ID пользователя, архивирующего проект
     * @return true, если архивация прошла успешно.
     */
    virtual bool archiveProject(int64_t id, int64_t userId) = 0;

    /**
     * @brief Восстанавливает проект из архива.
     * @param id Идентификатор проекта.
     * @param userId ID пользователя, восстанавливающего проект
     * @return true, если восстановление прошло успешно.
     */
    virtual bool restoreProject(int64_t id, int64_t userId) = 0;
};

} // namespace services
} // namespace server
