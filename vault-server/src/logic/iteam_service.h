#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/team.h"

namespace server::services
{

/**
 * @brief Структура для возврата результатов пагинированного списка команд.
 */
struct TeamsPage
{
    std::vector<dto::Team> teams;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления командами.
 */
class ITeamService
{
public:
    virtual ~ITeamService() = default;

    /**
     * @brief Получает список команд с пагинацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param searchCaption Поиск по названию (опционально)
     * @return Страница с командами
     */
    virtual TeamsPage getTeams(
        int page,
        int pageSize,
        int64_t userId,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает команду по ID.
     * @param id Идентификатор команды
     * @param userId ID пользователя для проверки прав
     * @return DTO команды или std::nullopt
     */
    virtual std::optional<dto::Team> getTeam(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создает новую команду.
     * @param team DTO команды
     * @param userId ID пользователя для проверки прав
     * @return Созданная команда или std::nullopt при ошибке
     */
    virtual std::optional<dto::Team> createTeam(
        const dto::Team& team,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую команду.
     * @param team DTO команды с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленная команда или std::nullopt при ошибке
     */
    virtual std::optional<dto::Team> updateTeam(
        const dto::Team& team,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет команду.
     * @param id Идентификатор команды
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteTeam(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
