#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "common/dto/project_team.h"

namespace server::repositories
{

/**
 * @brief Интерфейс репозитория для работы со связями проектов и команд (ProjectTeam).
 */
class IProjectTeamRepository
{
public:
    virtual ~IProjectTeamRepository() = default;

    /**
     * @brief Получает список связей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param projectId Фильтр по проекту
     * @param teamId Фильтр по команде
     * @return Пара: вектор DTO связей и общее количество
     */
    virtual std::pair<std::vector<dto::ProjectTeam>, int64_t> findAll(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt
    ) = 0;

    /**
     * @brief Находит связь по ID.
     * @param id Идентификатор связи
     * @return DTO связи или std::nullopt
     */
    virtual std::optional<dto::ProjectTeam> findById(int64_t id) = 0;

    /**
     * @brief Проверяет существование связи для пары (projectId, teamId).
     * @param projectId Идентификатор проекта
     * @param teamId Идентификатор команды
     * @return true если существует
     */
    virtual bool exists(int64_t projectId, int64_t teamId) = 0;

    /**
     * @brief Создаёт новую связь проекта и команды.
     * @param projectTeam DTO связи
     * @return ID созданной записи или 0 при ошибке
     */
    virtual int64_t create(const dto::ProjectTeam& projectTeam) = 0;

    /**
     * @brief Удаляет связь по ID.
     * @param id Идентификатор записи
     * @return true если удаление успешно
     */
    virtual bool remove(int64_t id) = 0;
};

} // namespace server::repositories
