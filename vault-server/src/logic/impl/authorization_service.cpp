#include <mutex>

#include "common/log/log.h"

#include "repo/role_repository.h"
#include "repo/rule_item_type_repository.h"
#include "repo/rule_project_repository.h"
#include "repo/rule_repository.h"
#include "repo/rule_state_repository.h"
#include "repo/team_repository.h"
#include "repo/user_repository.h"
#include "repo/user_team_role_repository.h"

#include "authorization_service.h"

namespace server::services
{

AuthorizationService::AuthorizationService(
    std::shared_ptr<repositories::IUserRepository> userRepo,
    std::shared_ptr<repositories::IRuleRepository> ruleRepo,
    std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
    std::shared_ptr<repositories::IRuleItemTypeRepository> ruleItemTypeRepo,
    std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
    std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
    std::shared_ptr<repositories::ITeamRepository> teamRepo,
    std::shared_ptr<repositories::IRoleRepository> roleRepo
)
    : m_userRepo(std::move(userRepo))
    , m_ruleRepo(std::move(ruleRepo))
    , m_ruleProjectRepo(std::move(ruleProjectRepo))
    , m_ruleItemTypeRepo(std::move(ruleItemTypeRepo))
    , m_ruleStateRepo(std::move(ruleStateRepo))
    , m_userTeamRoleRepo(std::move(userTeamRoleRepo))
    , m_teamRepo(std::move(teamRepo))
    , m_roleRepo(std::move(roleRepo))
{
}

bool AuthorizationService::isSuperAdmin(int64_t userId)
{
    auto& perms = getPermissions(userId);
    return perms.isSuperAdmin;
}

AuthzResult AuthorizationService::canReadProject(int64_t userId, int64_t projectId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    return checkProjectRule(
        perms,
        projectId,
        [](const dto::RuleProject& rule)
        {
            return rule.isReader.value_or(false);
        },
        "Недостаточно прав для чтения проекта"
    );
}

AuthzResult AuthorizationService::canWriteToProject(int64_t userId, int64_t projectId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    return checkProjectRule(
        perms,
        projectId,
        [](const dto::RuleProject& rule)
        {
            return rule.isWriter.value_or(false);
        },
        "Недостаточно прав для изменения элементов проекта"
    );
}

AuthzResult AuthorizationService::canEditProject(int64_t userId, int64_t projectId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    return checkProjectRule(
        perms,
        projectId,
        [](const dto::RuleProject& rule)
        {
            return rule.isProjectEditor.value_or(false);
        },
        "Недостаточно прав для редактирования проекта"
    );
}

AuthzResult AuthorizationService::canEditPhases(int64_t userId, int64_t projectId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    return checkProjectRule(
        perms,
        projectId,
        [](const dto::RuleProject& rule)
        {
            return rule.isPhaseEditor.value_or(false);
        },
        "Недостаточно прав для редактирования фаз проекта"
    );
}

AuthzResult AuthorizationService::canEditBoards(int64_t userId, int64_t projectId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    return checkProjectRule(
        perms,
        projectId,
        [](const dto::RuleProject& rule)
        {
            return rule.isBoardEditor.value_or(false);
        },
        "Недостаточно прав для редактирования досок проекта"
    );
}

AuthzResult AuthorizationService::canReadItemType(int64_t userId, int64_t projectId, int64_t itemTypeId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    // Сначала проверяем проектное право на чтение
    auto projectResult = canReadProject(userId, projectId);
    if (!projectResult.granted)
    {
        return projectResult;
    }

    return checkItemTypeRule(
        perms,
        projectId,
        itemTypeId,
        [](const dto::RuleItemType& rule)
        {
            return rule.isReader.value_or(false);
        },
        "Недостаточно прав для чтения элементов данного типа"
    );
}

AuthzResult AuthorizationService::canWriteItemType(int64_t userId, int64_t projectId, int64_t itemTypeId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    // Сначала проверяем проектное право на запись
    auto projectResult = canWriteToProject(userId, projectId);
    if (!projectResult.granted)
    {
        return projectResult;
    }

    return checkItemTypeRule(
        perms,
        projectId,
        itemTypeId,
        [](const dto::RuleItemType& rule)
        {
            return rule.isWriter.value_or(false);
        },
        "Недостаточно прав для создания/изменения элементов данного типа"
    );
}

AuthzResult AuthorizationService::canTransitionToState(int64_t userId, int64_t projectId, int64_t stateId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    // Сначала проверяем проектное право на запись
    auto projectResult = canWriteToProject(userId, projectId);
    if (!projectResult.granted)
    {
        return projectResult;
    }

    return checkStateRule(
        perms,
        projectId,
        stateId,
        [](const dto::RuleState& rule)
        {
            return rule.isStateAllowed.value_or(false);
        },
        "Недостаточно прав для перевода элемента в указанное состояние"
    );
}

AuthzResult AuthorizationService::canCreateRootProject(int64_t userId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        return AuthzResult { true };
    }

    if (perms.canCreateRootProject)
    {
        return AuthzResult { true };
    }

    return AuthzResult {
        false,
        403,
        "Недостаточно прав для создания корневого проекта"
    };
}

std::vector<int64_t> AuthorizationService::getReadableProjectIds(int64_t userId)
{
    auto& perms = getPermissions(userId);
    if (perms.isSuperAdmin)
    {
        // Для супер-админа возвращаем пустой список, что означает "все проекты"
        return {};
    }
    return perms.readableProjects;
}

void AuthorizationService::invalidateCache(int64_t userId)
{
    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    m_cache.erase(userId);
    LOG_DEBUG << "Очищен кэш прав для пользователя " << userId;
}

AuthorizationService::UserPermissions& AuthorizationService::getPermissions(int64_t userId)
{
    {
        std::shared_lock<std::shared_mutex> lock(m_cacheMutex);
        auto it = m_cache.find(userId);
        if (it != m_cache.end())
        {
            return it->second;
        }
    }

    // Загружаем из БД
    auto perms = loadPermissions(userId);

    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    return m_cache[userId] = std::move(perms);
}

AuthorizationService::UserPermissions AuthorizationService::loadPermissions(int64_t userId)
{
    UserPermissions perms;

    // Проверяем, существует ли пользователь
    auto user = m_userRepo->findById(userId);
    if (!user)
    {
        LOG_WARN << "Пользователь " << userId << " не найден при загрузке прав";
        return perms;
    }

    perms.isSuperAdmin = user->isSuperAdmin.value_or(false);
    if (perms.isSuperAdmin)
    {
        // Супер-администратору не нужно загружать остальные права
        return perms;
    }

    // Загружаем права на проекты
    loadProjectPermissions(userId, perms);

    // Загружаем права на типы элементов
    loadItemTypePermissions(userId, perms);

    // Загружаем права на состояния
    loadStatePermissions(userId, perms);

    // Загружаем право на создание корневых проектов
    int64_t roleId = getUserRoleId(userId);
    if (roleId > 0)
    {
        auto rule = m_ruleRepo->findByRoleId(roleId);
        if (rule)
        {
            perms.canCreateRootProject = rule->isRootProjectCreator.value_or(false);
        }
    }

    return perms;
}

void AuthorizationService::loadProjectPermissions(int64_t userId, UserPermissions& perms)
{
    // Получаем все команды пользователя
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    for (const auto& utr : userTeamRoles)
    {
        // Получаем правило для роли пользователя в этой команде
        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule)
        {
            continue;
        }

        // Получаем все права на проекты для этого правила
        auto projectRules = m_ruleProjectRepo->findByRuleId(*rule->id);
        for (const auto& pr : projectRules)
        {
            int64_t projectId = *pr.projectId;

            // Мержим права: если уже есть, объединяем (OR)
            auto it = perms.projectRules.find(projectId);
            if (it == perms.projectRules.end())
            {
                perms.projectRules[projectId] = pr;
                if (pr.isReader.value_or(false))
                {
                    perms.readableProjects.push_back(projectId);
                }
            }
            else
            {
                // Объединяем права (логическое ИЛИ)
                dto::RuleProject& existing = it->second;
                if (pr.isReader.value_or(false))
                    existing.isReader = true;
                if (pr.isWriter.value_or(false))
                    existing.isWriter = true;
                if (pr.isProjectEditor.value_or(false))
                    existing.isProjectEditor = true;
                if (pr.isPhaseEditor.value_or(false))
                    existing.isPhaseEditor = true;
                if (pr.isBoardEditor.value_or(false))
                    existing.isBoardEditor = true;
            }
        }
    }
}

void AuthorizationService::loadItemTypePermissions(int64_t userId, UserPermissions& perms)
{
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    for (const auto& utr : userTeamRoles)
    {
        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule)
        {
            continue;
        }

        auto itemTypeRules = m_ruleItemTypeRepo->findByRuleId(*rule->id);
        for (const auto& itr : itemTypeRules)
        {
            int64_t itemTypeId = *itr.itemTypeId;
            auto existingIt = perms.itemTypeRules.find(itemTypeId);
            if (existingIt == perms.itemTypeRules.end())
            {
                perms.itemTypeRules[itemTypeId] = itr;
            }
            else
            {
                dto::RuleItemType& existing = existingIt->second;
                if (itr.isReader.value_or(false))
                    existing.isReader = true;
                if (itr.isWriter.value_or(false))
                    existing.isWriter = true;
            }
        }
    }
}

void AuthorizationService::loadStatePermissions(int64_t userId, UserPermissions& perms)
{
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    for (const auto& utr : userTeamRoles)
    {
        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule)
        {
            continue;
        }

        auto stateRules = m_ruleStateRepo->findByRuleId(*rule->id);
        for (const auto& sr : stateRules)
        {
            int64_t stateId = *sr.stateId;
            auto existingIt = perms.stateRules.find(stateId);
            if (existingIt == perms.stateRules.end())
            {
                perms.stateRules[stateId] = sr;
            }
            else
            {
                if (sr.isStateAllowed.value_or(false))
                {
                    existingIt->second.isStateAllowed = true;
                }
            }
        }
    }
}

int64_t AuthorizationService::getUserRoleId(int64_t userId)
{
    // Упрощенная логика: берем первую роль пользователя в первой команде
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);
    if (!userTeamRoles.empty())
    {
        return *userTeamRoles[0].roleId;
    }
    return 0;
}

AuthzResult AuthorizationService::checkProjectRule(
    const UserPermissions& perms,
    int64_t projectId,
    std::function<bool(const dto::RuleProject&)> checker,
    const std::string& errorMessage
)
{
    auto it = perms.projectRules.find(projectId);
    if (it != perms.projectRules.end() && checker(it->second))
    {
        return AuthzResult { true };
    }

    // TODO: Проверяем рекурсивно родительские проекты
    // Для этого нужно загрузить информацию о родителе проекта

    return AuthzResult { false, 403, errorMessage };
}

AuthzResult AuthorizationService::checkItemTypeRule(
    const UserPermissions& perms,
    int64_t /*projectId*/,
    int64_t itemTypeId,
    std::function<bool(const dto::RuleItemType&)> checker,
    const std::string& errorMessage
)
{
    auto it = perms.itemTypeRules.find(itemTypeId);
    if (it != perms.itemTypeRules.end() && checker(it->second))
    {
        return AuthzResult { true };
    }

    return AuthzResult { false, 403, errorMessage };
}

AuthzResult AuthorizationService::checkStateRule(
    const UserPermissions& perms,
    int64_t /*projectId*/,
    int64_t stateId,
    std::function<bool(const dto::RuleState&)> checker,
    const std::string& errorMessage
)
{
    auto it = perms.stateRules.find(stateId);
    if (it != perms.stateRules.end() && checker(it->second))
    {
        return AuthzResult { true };
    }

    return AuthzResult { false, 403, errorMessage };
}

} // namespace server::services
