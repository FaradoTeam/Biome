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
    if (!m_userRepo
        || !m_ruleRepo
        || !m_ruleProjectRepo
        || !m_ruleItemTypeRepo
        || !m_ruleStateRepo
        || !m_userTeamRoleRepo
        || !m_teamRepo
        || !m_roleRepo)
    {
        throw std::runtime_error(
            "AuthorizationService: один или несколько репозиториев не инициализированы"
        );
    }
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
    auto it = m_cache.find(userId);
    if (it != m_cache.end())
    {
        m_cache.erase(it);
        LOG_INFO << "Инвалидирован кэш прав для пользователя " << userId;
    }
    else
    {
        LOG_DEBUG << "Пользователь " << userId << " не найден в кэше прав";
    }
}

std::vector<int64_t> AuthorizationService::getUserIdsByRoleId(int64_t roleId)
{
    std::vector<int64_t> userIds;
    auto userTeamRoles = m_userTeamRoleRepo->findByRoleId(roleId);
    for (const auto& utr : userTeamRoles)
    {
        if (utr.userId.has_value())
        {
            userIds.push_back(*utr.userId);
        }
    }
    return userIds;
}

AuthorizationService::UserPermissions& AuthorizationService::getPermissions(int64_t userId)
{
    // Сначала проверяем кэш
    {
        std::shared_lock<std::shared_mutex> lock(m_cacheMutex);
        auto it = m_cache.find(userId);
        if (it != m_cache.end())
        {
            return it->second;
        }
    }

    // Загружаем из БД
    LOG_DEBUG << "Загрузка прав для пользователя " << userId << " из БД";
    auto perms = loadPermissions(userId);

    // Сохраняем в кэш
    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    auto& result = m_cache[userId];
    result = std::move(perms);

    LOG_DEBUG
        << "Права для пользователя " << userId
        << " загружены. Проектов: " << result.readableProjects.size()
        << ", СуперАдмин: " << result.isSuperAdmin;

    return result;
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
        LOG_DEBUG
            << "Пользователь " << userId << " является супер-администратором";
        return perms;
    }

    // Загружаем права на проекты
    loadProjectPermissions(userId, perms);

    // Загружаем права на типы элементов
    loadItemTypePermissions(userId, perms);

    // Загружаем права на состояния
    loadStatePermissions(userId, perms);

    // Загружаем право на создание корневых проектов (через правило)
    auto roleIds = getUserRoleIds(userId);
    for (int64_t roleId : roleIds)
    {
        auto rule = m_ruleRepo->findByRoleId(roleId);
        if (rule && rule->isRootProjectCreator.has_value() && *rule->isRootProjectCreator)
        {
            perms.canCreateRootProject = true;
            LOG_DEBUG
                << "Пользователь " << userId
                << " получил право на создание корневых проектов через роль " << roleId;
            break;
        }
    }

    LOG_DEBUG
        << "Загрузка прав для пользователя " << userId << " завершена. "
        << "Доступно проектов: " << perms.readableProjects.size();

    return perms;
}

void AuthorizationService::loadProjectPermissions(int64_t userId, UserPermissions& perms)
{
    // Получаем все команды пользователя
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    LOG_DEBUG
        << "Пользователь " << userId
        << " состоит в " << userTeamRoles.size() << " командах";

    for (const auto& utr : userTeamRoles)
    {
        if (!utr.roleId.has_value())
        {
            LOG_WARN
                << "UserTeamRole " << utr.id.value_or(0) << " не имеет roleId";
            continue;
        }

        // Получаем правило для роли пользователя в этой команде
        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule || !rule->id.has_value())
        {
            LOG_DEBUG << "Нет правила для роли " << *utr.roleId;
            continue;
        }

        // Получаем все права на проекты для этого правила
        auto projectRules = m_ruleProjectRepo->findByRuleId(*rule->id);

        LOG_DEBUG
            << "Для правила " << *rule->id
            << " найдено " << projectRules.size() << " записей RuleProject";

        for (const auto& pr : projectRules)
        {
            if (!pr.projectId.has_value())
            {
                LOG_WARN
                    << "RuleProject " << pr.id.value_or(0) << " не имеет projectId";
                continue;
            }

            int64_t projectId = *pr.projectId;

            // Мержим права: если уже есть, объединяем (OR)
            auto it = perms.projectRules.find(projectId);
            if (it == perms.projectRules.end())
            {
                perms.projectRules[projectId] = pr;
                if (pr.isReader.value_or(false))
                {
                    perms.readableProjects.push_back(projectId);
                    LOG_DEBUG
                        << "Пользователь " << userId
                        << " получил доступ на чтение к проекту " << projectId;
                }
                if (pr.isPhaseEditor.value_or(false))
                {
                    LOG_DEBUG
                        << "Пользователь " << userId
                        << " получил право isPhaseEditor для проекта " << projectId;
                }
            }
            else
            {
                // Объединяем права (логическое ИЛИ)
                dto::RuleProject& existing = it->second;
                if (pr.isReader.value_or(false))
                {
                    existing.isReader = true;
                }
                if (pr.isWriter.value_or(false))
                {
                    existing.isWriter = true;
                }
                if (pr.isProjectEditor.value_or(false))
                {
                    existing.isProjectEditor = true;
                }
                if (pr.isPhaseEditor.value_or(false))
                {
                    existing.isPhaseEditor = true;
                }
                if (pr.isBoardEditor.value_or(false))
                {
                    existing.isBoardEditor = true;
                }
                LOG_DEBUG
                    << "Объединены права для пользователя " << userId
                    << " на проект " << projectId;
            }
        }
    }
}

void AuthorizationService::loadItemTypePermissions(int64_t userId, UserPermissions& perms)
{
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    for (const auto& utr : userTeamRoles)
    {
        if (!utr.roleId.has_value())
            continue;

        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule || !rule->id.has_value())
            continue;

        auto itemTypeRules = m_ruleItemTypeRepo->findByRuleId(*rule->id);
        for (const auto& itr : itemTypeRules)
        {
            if (!itr.itemTypeId.has_value())
                continue;

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
        if (!utr.roleId.has_value())
            continue;

        auto rule = m_ruleRepo->findByRoleId(*utr.roleId);
        if (!rule || !rule->id.has_value())
            continue;

        auto stateRules = m_ruleStateRepo->findByRuleId(*rule->id);
        for (const auto& sr : stateRules)
        {
            if (!sr.stateId.has_value())
                continue;

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

std::vector<int64_t> AuthorizationService::getUserRoleIds(int64_t userId)
{
    std::vector<int64_t> roleIds;
    auto userTeamRoles = m_userTeamRoleRepo->findByUserId(userId);

    for (const auto& utr : userTeamRoles)
    {
        if (utr.roleId.has_value())
        {
            roleIds.push_back(*utr.roleId);
        }
    }

    return roleIds;
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
        LOG_DEBUG << "Проверка прав на проект " << projectId << " успешна";
        return AuthzResult { true };
    }

    LOG_DEBUG
        << "Проверка прав на проект " << projectId
        << " не пройдена: " << errorMessage;
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
