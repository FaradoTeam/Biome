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
        throw std::runtime_error("TeamService: репозиторий команд не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("TeamService: сервис авторизации не инициализирован");
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
        LOG_WARN << "createTeam: название команды обязательно";
        return std::nullopt;
    }

    int64_t newId = m_teamRepo->create(team);
    if (newId <= 0)
    {
        LOG_ERROR << "createTeam: не удалось создать команду";
        return std::nullopt;
    }

    LOG_INFO << "Команда создана: id=" << newId << ", название=" << *team.caption;

    return m_teamRepo->findById(newId);
}

std::optional<dto::Team> TeamService::updateTeam(const dto::Team& team)
{
    if (!team.id.has_value())
    {
        LOG_WARN << "updateTeam: отсутствует id";
        return std::nullopt;
    }

    auto existing = m_teamRepo->findById(*team.id);
    if (!existing)
    {
        LOG_WARN << "updateTeam: команда не найдена, id=" << *team.id;
        return std::nullopt;
    }

    if (!m_teamRepo->update(team))
    {
        LOG_ERROR << "updateTeam: не удалось обновить команду id=" << *team.id;
        return std::nullopt;
    }

    LOG_INFO << "Команда обновлена: id=" << *team.id;

    return m_teamRepo->findById(*team.id);
}

bool TeamService::deleteTeam(int64_t id)
{
    auto existing = m_teamRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteTeam: команда не найдена, id=" << id;
        return false;
    }

    // TODO: проверить, что команда не используется в ProjectTeam и UserTeamRole
    // Это можно сделать через соответствующие репозитории
    if (!m_teamRepo->remove(id))
    {
        LOG_ERROR << "deleteTeam: не удалось удалить команду id=" << id;
        return false;
    }

    LOG_INFO << "Команда удалена: id=" << id;

    // При удалении команды нужно инвалидировать кэш всех пользователей, состоявших в ней
    invalidateUsersByTeamId(id);

    return true;
}

void TeamService::invalidateUsersByTeamId(int64_t teamId)
{
    LOG_DEBUG << "Инвалидация кэша для пользователей команды " << teamId;
    // TODO: Реализовать получение пользователей по teamId
}

} // namespace server::services
