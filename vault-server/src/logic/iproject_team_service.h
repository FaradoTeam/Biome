#pragma once

#include <optional>
#include <vector>

#include "common/dto/project_team.h"

namespace server::services
{

/**
 * @brief Результат операции со связью проекта и команды.
 */
struct ProjectTeamResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница со связями проектов и команд.
 */
struct ProjectTeamsPage
{
    std::vector<dto::ProjectTeam> items;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления связями проектов и команд.
 */
class IProjectTeamService
{
public:
    virtual ~IProjectTeamService() = default;

    /**
     * @brief Получает список связей с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param projectId Фильтр по проекту
     * @param teamId Фильтр по команде
     * @return Страница со связями
     */
    virtual ProjectTeamsPage getProjectTeams(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt
    ) = 0;

    /**
     * @brief Получает связь по ID.
     * @param id Идентификатор связи
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ProjectTeam> getProjectTeam(int64_t id) = 0;

    /**
     * @brief Создаёт новую связь проекта и команды.
     * @param projectTeam DTO связи
     * @param userId ID пользователя для проверки прав
     * @return Созданная связь или std::nullopt при ошибке
     */
    virtual std::optional<dto::ProjectTeam> createProjectTeam(
        const dto::ProjectTeam& projectTeam,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет связь.
     * @param id Идентификатор связи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ProjectTeamResult deleteProjectTeam(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
