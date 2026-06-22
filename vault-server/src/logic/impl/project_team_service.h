#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iproject_team_service.h"

#include "repo/project_repository.h"
#include "repo/project_team_repository.h"
#include "repo/team_repository.h"

namespace server::services
{

/**
 * @brief Реализация сервиса для управления связями проектов и команд.
 */
class ProjectTeamService final : public IProjectTeamService
{
public:
    ProjectTeamService(
        std::shared_ptr<repositories::IProjectTeamRepository> projectTeamRepo,
        std::shared_ptr<repositories::IProjectRepository> projectRepo,
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    ProjectTeamsPage getProjectTeams(
        int page,
        int pageSize,
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> teamId = std::nullopt
    ) override;

    std::optional<dto::ProjectTeam> getProjectTeam(int64_t id) override;

    std::optional<dto::ProjectTeam> createProjectTeam(
        const dto::ProjectTeam& projectTeam,
        int64_t userId
    ) override;

    ProjectTeamResult deleteProjectTeam(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет, имеет ли пользователь право редактирования проекта.
     */
    bool canEditProject(int64_t userId, int64_t projectId);

    std::shared_ptr<repositories::IProjectTeamRepository> m_projectTeamRepo;
    std::shared_ptr<repositories::IProjectRepository> m_projectRepo;
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
