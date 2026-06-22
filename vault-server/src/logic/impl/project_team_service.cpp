#include "project_team_service.h"
#include "common/log/log.h"

namespace server::services
{

ProjectTeamService::ProjectTeamService(
    std::shared_ptr<repositories::IProjectTeamRepository> projectTeamRepo,
    std::shared_ptr<repositories::IProjectRepository> projectRepo,
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_projectTeamRepo(std::move(projectTeamRepo))
    , m_projectRepo(std::move(projectRepo))
    , m_teamRepo(std::move(teamRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_projectTeamRepo || !m_projectRepo || !m_teamRepo)
    {
        throw std::runtime_error(
            "ProjectTeamService: репозитории не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error(
            "ProjectTeamService: сервис авторизации не инициализирован"
        );
    }
}

ProjectTeamsPage ProjectTeamService::getProjectTeams(
    int page,
    int pageSize,
    std::optional<int64_t> projectId,
    std::optional<int64_t> teamId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [items, total] = m_projectTeamRepo->findAll(page, pageSize, projectId, teamId);
    return { items, total };
}

std::optional<dto::ProjectTeam> ProjectTeamService::getProjectTeam(
    int64_t id
)
{
    return m_projectTeamRepo->findById(id);
}

std::optional<dto::ProjectTeam> ProjectTeamService::createProjectTeam(
    const dto::ProjectTeam& projectTeam,
    int64_t userId
)
{
    // Проверка обязательных полей
    if (!projectTeam.projectId.has_value() || !projectTeam.teamId.has_value())
    {
        LOG_WARN << "createProjectTeam: обязательны projectId и teamId";
        return std::nullopt;
    }

    // Проверка прав: требуется право на редактирование проекта
    if (!canEditProject(userId, *projectTeam.projectId))
    {
        LOG_WARN
            << "createProjectTeam: пользователь "
            << userId
            << " не имеет прав на редактирование проекта "
            << *projectTeam.projectId;
        return std::nullopt;
    }

    // Проверка существования проекта и команды
    if (!m_projectRepo->exists(*projectTeam.projectId))
    {
        LOG_WARN << "createProjectTeam: проект не найден, projectId="
                 << *projectTeam.projectId;
        return std::nullopt;
    }
    if (!m_teamRepo->exists(*projectTeam.teamId))
    {
        LOG_WARN << "createProjectTeam: команда не найдена, teamId="
                 << *projectTeam.teamId;
        return std::nullopt;
    }

    // Проверка уникальности пары
    if (m_projectTeamRepo->exists(*projectTeam.projectId, *projectTeam.teamId))
    {
        LOG_WARN
            << "createProjectTeam: связь проекта и команды уже существует";
        return std::nullopt;
    }

    const int64_t newId = m_projectTeamRepo->create(projectTeam);
    if (newId <= 0)
    {
        LOG_ERROR << "createProjectTeam: не удалось создать связь";
        return std::nullopt;
    }

    LOG_INFO
        << "Связь проекта и команды создана: id=" << newId
        << ", projectId=" << *projectTeam.projectId
        << ", teamId=" << *projectTeam.teamId
        << ", пользователь=" << userId;

    return m_projectTeamRepo->findById(newId);
}

ProjectTeamResult ProjectTeamService::deleteProjectTeam(
    int64_t id,
    int64_t userId
)
{
    ProjectTeamResult result;

    auto existing = m_projectTeamRepo->findById(id);
    if (!existing)
    {
        result.errorMessage = "Связь не найдена";
        result.errorCode = 404;
        LOG_WARN << "deleteProjectTeam: связь не найдена, id=" << id;
        return result;
    }

    // Проверка прав: требуется право на редактирование проекта
    if (!canEditProject(userId, *existing->projectId))
    {
        result.errorMessage = "Недостаточно прав для удаления связи проекта и команды";
        result.errorCode = 403;
        LOG_WARN
            << "deleteProjectTeam: пользователь "
            << userId
            << " не имеет прав на редактирование проекта "
            << *existing->projectId;
        return result;
    }

    if (!m_projectTeamRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить связь";
        result.errorCode = 500;
        LOG_ERROR << "deleteProjectTeam: не удалось удалить связь id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Связь проекта и команды удалена: id=" << id
        << ", пользователь=" << userId;
    return result;
}

bool ProjectTeamService::canEditProject(int64_t userId, int64_t projectId)
{
    // Супер-администратор имеет все права
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Проверяем право на редактирование проекта
    auto authz = m_authzService->canEditProject(userId, projectId);
    if (!authz.granted)
    {
        LOG_DEBUG
            << "canEditProject: пользователь " << userId
            << " не имеет прав на редактирование проекта " << projectId;
        return false;
    }

    return true;
}

} // namespace server::services
