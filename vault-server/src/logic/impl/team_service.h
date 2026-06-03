#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iteam_service.h"

#include "repo/team_repository.h"
#include "repo/user_team_role_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для управления командами.
 */
class TeamService final : public ITeamService
{
public:
    /**
     * @brief Конструктор.
     * @param teamRepo Репозиторий команд
     * @param userTeamRoleRepo Репозиторий назначений пользователей
     * @param authzService Сервис авторизации для проверки прав
     */
    TeamService(
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // ITeamService
    TeamsPage getTeams(
        int page,
        int pageSize,
        int64_t userId,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::Team> getTeam(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::Team> createTeam(
        const dto::Team& team,
        int64_t userId
    ) override;

    std::optional<dto::Team> updateTeam(
        const dto::Team& team,
        int64_t userId
    ) override;

    bool deleteTeam(
        int64_t id,
        int64_t userId
    ) override;

private:
    void invalidateUsersByTeamId(int64_t teamId);

private:
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<repositories::IUserTeamRoleRepository> m_userTeamRoleRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
