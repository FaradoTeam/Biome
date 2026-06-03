#include "common/log/log.h"

#include "team_service.h"

namespace server::services
{

TeamService::TeamService(
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_teamRepo(std::move(teamRepo))
    , m_userTeamRoleRepo(std::move(userTeamRoleRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_teamRepo)
    {
        throw std::runtime_error("TeamService: репозиторий команд не инициализирован");
    }
    if (!m_userTeamRoleRepo)
    {
        throw std::runtime_error("TeamService: репозиторий UserTeamRole не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("TeamService: сервис авторизации не инициализирован");
    }
}

TeamsPage TeamService::getTeams(int page, int pageSize, int64_t /*userId*/, const std::string& searchCaption)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [teams, total] = m_teamRepo->findAll(page, pageSize, searchCaption);
    return { teams, total };
}

std::optional<dto::Team> TeamService::getTeam(int64_t id, int64_t /*userId*/)
{
    return m_teamRepo->findById(id);
}

std::optional<dto::Team> TeamService::createTeam(
    const dto::Team& team,
    int64_t userId
)
{
    // Только супер-админ может создавать команды
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createTeam: пользователь " << userId << " не имеет прав на создание команд";
        return std::nullopt;
    }

    if (!team.caption.has_value() || team.caption->empty())
    {
        LOG_WARN << "createTeam: название команды обязательно";
        return std::nullopt;
    }

    const int64_t newId = m_teamRepo->create(team);
    if (newId <= 0)
    {
        LOG_ERROR << "createTeam: не удалось создать команду";
        return std::nullopt;
    }

    LOG_INFO
        << "Команда создана: id=" << newId
        << ", название=" << *team.caption
        << ", пользователь=" << userId;

    return m_teamRepo->findById(newId);
}

std::optional<dto::Team> TeamService::updateTeam(
    const dto::Team& team,
    int64_t userId
)
{
    // Только супер-админ может обновлять команды
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateTeam: пользователь " << userId << " не имеет прав на обновление команд";
        return std::nullopt;
    }

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

    const int64_t oldTeamId = *team.id;

    if (!m_teamRepo->update(team))
    {
        LOG_ERROR << "updateTeam: не удалось обновить команду id=" << *team.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Команда обновлена: id=" << *team.id
        << ", пользователь=" << userId;

    // Инвалидируем кэш для всех пользователей этой команды
    invalidateUsersByTeamId(oldTeamId);

    return m_teamRepo->findById(*team.id);
}

bool TeamService::deleteTeam(int64_t id, int64_t userId)
{
    // Только супер-админ может удалять команды
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteTeam: пользователь " << userId << " не имеет прав на удаление команд";
        return false;
    }

    auto existing = m_teamRepo->findById(id);
    if (!existing)
    {
        LOG_WARN << "deleteTeam: команда не найдена, id=" << id;
        return false;
    }

    // TODO: проверить, что команда не используется в ProjectTeam
    if (!m_teamRepo->remove(id))
    {
        LOG_ERROR << "deleteTeam: не удалось удалить команду id=" << id;
        return false;
    }

    LOG_INFO
        << "Команда удалена: id=" << id
        << ", пользователь=" << userId;

    // При удалении команды нужно инвалидировать кэш всех пользователей, состоявших в ней
    invalidateUsersByTeamId(id);

    return true;
}

void TeamService::invalidateUsersByTeamId(int64_t teamId)
{
    auto userTeamRoles = m_userTeamRoleRepo->findByTeamId(teamId);
    LOG_DEBUG << "Инвалидация кэша для " << userTeamRoles.size() << " пользователей команды " << teamId;

    for (const auto& utr : userTeamRoles)
    {
        if (utr.userId.has_value())
        {
            m_authzService->invalidateCache(*utr.userId);
            LOG_DEBUG << "Инвалидирован кэш для пользователя " << *utr.userId;
        }
    }
}

} // namespace server::services
