#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "common/dto/rule_item_type.h"
#include "common/dto/rule_project.h"
#include "common/dto/rule_state.h"

#include "logic/iauthorization_service.h"

namespace server::repositories
{
class IUserRepository;
class IRuleRepository;
class IRuleProjectRepository;
class IRuleItemTypeRepository;
class IRuleStateRepository;
class IUserTeamRoleRepository;
class ITeamRepository;
class IRoleRepository;
}

namespace server::services
{

/**
 * @brief Реализация сервиса авторизации.
 *
 * Кэширует права пользователей для повышения производительности.
 */
class AuthorizationService final : public IAuthorizationService
{
public:
    AuthorizationService(
        std::shared_ptr<repositories::IUserRepository> userRepo,
        std::shared_ptr<repositories::IRuleRepository> ruleRepo,
        std::shared_ptr<repositories::IRuleProjectRepository> ruleProjectRepo,
        std::shared_ptr<repositories::IRuleItemTypeRepository> ruleItemTypeRepo,
        std::shared_ptr<repositories::IRuleStateRepository> ruleStateRepo,
        std::shared_ptr<repositories::IUserTeamRoleRepository> userTeamRoleRepo,
        std::shared_ptr<repositories::ITeamRepository> teamRepo,
        std::shared_ptr<repositories::IRoleRepository> roleRepo
    );

    // IAuthorizationService
    bool isSuperAdmin(int64_t userId) override;

    AuthzResult canReadProject(int64_t userId, int64_t projectId) override;
    AuthzResult canWriteToProject(int64_t userId, int64_t projectId) override;
    AuthzResult canEditProject(int64_t userId, int64_t projectId) override;
    AuthzResult canEditPhases(int64_t userId, int64_t projectId) override;
    AuthzResult canEditBoards(int64_t userId, int64_t projectId) override;

    AuthzResult canReadItemType(int64_t userId, int64_t projectId, int64_t itemTypeId) override;
    AuthzResult canWriteItemType(int64_t userId, int64_t projectId, int64_t itemTypeId) override;

    AuthzResult canTransitionToState(int64_t userId, int64_t projectId, int64_t stateId) override;

    AuthzResult canCreateRootProject(int64_t userId) override;

    std::vector<int64_t> getReadableProjectIds(int64_t userId) override;

    void invalidateCache(int64_t userId) override;

    std::vector<int64_t> getUserIdsByRoleId(int64_t roleId) override;

private:
    // Структура для хранения прав пользователя
    struct UserPermissions
    {
        bool isSuperAdmin = false;
        bool canCreateRootProject = false;

        // Права на проекты: projectId -> RuleProject права
        std::unordered_map<int64_t, dto::RuleProject> projectRules;

        // Права на типы элементов: itemTypeId -> RuleItemType
        std::unordered_map<int64_t, dto::RuleItemType> itemTypeRules;

        // Права на состояния: stateId -> RuleState
        std::unordered_map<int64_t, dto::RuleState> stateRules;

        // Список проектов, доступных для чтения (кэш)
        std::vector<int64_t> readableProjects;
    };

    /**
     * @brief Загружает или возвращает из кэша права пользователя.
     */
    UserPermissions& getPermissions(int64_t userId);

    /**
     * @brief Загружает права пользователя из БД.
     */
    UserPermissions loadPermissions(int64_t userId);

    /**
     * @brief Загружает права на проекты для пользователя.
     */
    void loadProjectPermissions(int64_t userId, UserPermissions& perms);

    /**
     * @brief Загружает права на типы элементов для пользователя.
     */
    void loadItemTypePermissions(int64_t userId, UserPermissions& perms);

    /**
     * @brief Загружает права на состояния для пользователя.
     */
    void loadStatePermissions(int64_t userId, UserPermissions& perms);

    /**
     * @brief Получает ID роли пользователя.
     */
    std::vector<int64_t> getUserRoleIds(int64_t userId);

    /**
     * @brief Проверяет проектное право.
     */
    AuthzResult checkProjectRule(
        const UserPermissions& perms,
        int64_t projectId,
        std::function<bool(const dto::RuleProject&)> checker,
        const std::string& errorMessage
    );

    /**
     * @brief Проверяет право на тип элемента.
     */
    AuthzResult checkItemTypeRule(
        const UserPermissions& perms,
        int64_t projectId,
        int64_t itemTypeId,
        std::function<bool(const dto::RuleItemType&)> checker,
        const std::string& errorMessage
    );

    /**
     * @brief Проверяет право на состояние.
     */
    AuthzResult checkStateRule(
        const UserPermissions& perms,
        int64_t projectId,
        int64_t stateId,
        std::function<bool(const dto::RuleState&)> checker,
        const std::string& errorMessage
    );

    // Репозитории
    std::shared_ptr<repositories::IUserRepository> m_userRepo;
    std::shared_ptr<repositories::IRuleRepository> m_ruleRepo;
    std::shared_ptr<repositories::IRuleProjectRepository> m_ruleProjectRepo;
    std::shared_ptr<repositories::IRuleItemTypeRepository> m_ruleItemTypeRepo;
    std::shared_ptr<repositories::IRuleStateRepository> m_ruleStateRepo;
    std::shared_ptr<repositories::IUserTeamRoleRepository> m_userTeamRoleRepo;
    std::shared_ptr<repositories::ITeamRepository> m_teamRepo;
    std::shared_ptr<repositories::IRoleRepository> m_roleRepo;

    // Кэш прав пользователей
    std::unordered_map<int64_t, UserPermissions> m_cache;
    std::shared_mutex m_cacheMutex;
};

} // namespace server::services
