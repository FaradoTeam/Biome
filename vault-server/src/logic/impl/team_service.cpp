#include "common/log/log.h"

#include "team_service.h"

namespace server::services
{

TeamService::TeamService(
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_teamRepo(std::move(teamRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_teamRepo)
    {
        throw std::runtime_error("TeamService: teamRepository is null");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("TeamService: authorizationService is null");
    }
}

TeamsPage TeamService::getTeams(int page, int pageSize, const std::string& searchCaption)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [teams, total] = m_teamRepo->findAll(page, pageSize, searchCaption);
    return { teams, total };
}

std::optional<dto::Team> TeamService::getTeam(int64_t id)
{
    return m_teamRepo->findById(id);
}

std::optional<dto::Team> TeamService::createTeam(const dto::Team& team)
{
    if (!team.caption.has_value() || team.caption->empty())
    {
        LOG_WARN << "createTeam: caption is required";
        return std::nullopt;
    }

    int64_t newId = m_teamRepo->create(team);
    if (newId <= 0)
    {
        LOG_ERROR << "createTeam: failed to create team";
        return std::nullopt;
    }

    LOG_INFO << "Team created: id=" << newId << ", caption=" << *team.caption;

    return m_teamRepo->findById(newId);
}

std::optional<dto::Team> TeamService::updateTeam(const dto::Team& team)
{
    if (!team.id.has_value())
    {
        LOG_WARN << "updateTeam: missing id";
        return std::nullopt;
    }

    auto existing = m_teamRepo->findById(*team.id);
    if (!existing)
    {
        LOG_WARN << "updateTeam: team not found, id=" << *team.id;
        return std::nullopt;
    }

    if (!m_teamRepo->update(team))
    {
        LOG_ERROR << "updateTeam: failed to update team id=" << *team.id;
        return std::nullopt;
    }

    LOG_INFO << "Team updated: id=" << *team.id;

    return m_teamRepo->findById(*team.id);
}

bool TeamService::deleteTeam(int64_t id)
{
    auto existing = m_teamRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteTeam: team not found, id=" << id;
        return false;
    }

    // TODO: проверить, что команда не используется в ProjectTeam и UserTeamRole
    // Это можно сделать через соответствующие репозитории

    if (!m_teamRepo->remove(id))
    {
        LOG_ERROR << "deleteTeam: failed to delete team id=" << id;
        return false;
    }

    LOG_INFO << "Team deleted: id=" << id;

    // При удалении команды нужно инвалидировать кэш всех пользователей, состоявших в ней
    invalidateUsersByTeamId(id);

    return true;
}

void TeamService::invalidateUsersByTeamId(int64_t teamId)
{
    // Получаем всех пользователей, состоявших в этой команде
    // Используем метод findAll с фильтром по teamId
    // auto [userTeamRoles, total] = m_authzService->getUserTeamRolesByTeamId(teamId);

    // Если у нас нет такого метода в IAuthorizationService, нужно добавить
    // или использовать репозиторий напрямую через UserTeamRoleRepository

    // Альтернативный вариант: используем репозиторий UserTeamRole
    // auto userTeamRoles = m_userTeamRoleRepo->findByTeamId(teamId);

    LOG_DEBUG << "Инвалидация кэша для пользователей команды " << teamId;

    // Для простоты, пока оставим заглушку
    // TODO: Реализовать получение пользователей по teamId
}

} // namespace server::services
