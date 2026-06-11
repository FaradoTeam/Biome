#include <cstdlib>

#include <nlohmann/json.hpp>

#include "common/log/log.h"

#include "application.h"
#include "config.h"

#include "api/middleware/impl/auth_middleware.h"
#include "api/rest_server.h"

#include "logic/impl/auth_service.h"
#include "logic/impl/authorization_service.h"
#include "logic/impl/edge_service.h"
#include "logic/impl/field_type_possible_value_service.h"
#include "logic/impl/field_type_service.h"
#include "logic/impl/item_service.h"
#include "logic/impl/item_history_service.h"
#include "logic/impl/item_link_service.h"
#include "logic/impl/item_type_service.h"
#include "logic/impl/item_user_state_service.h"
#include "logic/impl/link_type_service.h"
#include "logic/impl/phase_service.h"
#include "logic/impl/project_service.h"
#include "logic/impl/role_menu_item_service.h"
#include "logic/impl/role_service.h"
#include "logic/impl/rule_item_type_service.h"
#include "logic/impl/rule_project_service.h"
#include "logic/impl/rule_service.h"
#include "logic/impl/rule_state_service.h"
#include "logic/impl/state_service.h"
#include "logic/impl/team_service.h"
#include "logic/impl/user_service.h"
#include "logic/impl/user_team_role_service.h"
#include "logic/impl/workflow_service.h"

#include "repo/sqlite/sqlite_edge_repository.h"
#include "repo/sqlite/sqlite_field_type_possible_value_repository.h"
#include "repo/sqlite/sqlite_field_type_repository.h"
#include "repo/sqlite/sqlite_item_field_repository.h"
#include "repo/sqlite/sqlite_item_repository.h"
#include "repo/sqlite/sqlite_item_history_repository.h"
#include "repo/sqlite/sqlite_item_link_repository.h"
#include "repo/sqlite/sqlite_item_type_repository.h"
#include "repo/sqlite/sqlite_item_user_state_repository.h"
#include "repo/sqlite/sqlite_link_type_repository.h"
#include "repo/sqlite/sqlite_phase_repository.h"
#include "repo/sqlite/sqlite_project_repository.h"
#include "repo/sqlite/sqlite_role_menu_item_repository.h"
#include "repo/sqlite/sqlite_role_repository.h"
#include "repo/sqlite/sqlite_rule_item_type_repository.h"
#include "repo/sqlite/sqlite_rule_project_repository.h"
#include "repo/sqlite/sqlite_rule_repository.h"
#include "repo/sqlite/sqlite_rule_state_repository.h"
#include "repo/sqlite/sqlite_state_repository.h"
#include "repo/sqlite/sqlite_team_repository.h"
#include "repo/sqlite/sqlite_user_repository.h"
#include "repo/sqlite/sqlite_user_team_role_repository.h"
#include "repo/sqlite/sqlite_workflow_repository.h"

#include "storage/database_factory.h"

namespace server
{

Application::Application()
    : m_isRunning(false)
{
}

Application::~Application()
{
    cleanup();
}

bool Application::initialize()
{
    LOG_INFO << "Инициализация приложения...";

    // === Инициализируем базу данных ===
    try
    {
        // Создаем базу данных через фабрику
        m_database = db::DatabaseFactory::create(db::DatabaseType::Sqlite);

        db::DatabaseConfig dbConfig;
        dbConfig["database"] = CONFIG.database.file;
        m_database->initialize(dbConfig);

        LOG_INFO << "База данных инициализирована: " << CONFIG.database.file;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Не удалось инициализировать базу данных: " << e.what();
        return false;
    }

    // === Создаем репозитории ===
    auto edgeRepository = std::make_shared<repositories::SqliteEdgeRepository>(m_database);
    auto fieldTypeRepository = std::make_shared<repositories::SqliteFieldTypeRepository>(m_database);
    auto fieldTypePossibleValueRepository = std::make_shared<repositories::SqliteFieldTypePossibleValueRepository>(m_database);
    auto itemRepository = std::make_shared<repositories::SqliteItemRepository>(m_database);
    auto itemFieldRepository = std::make_shared<repositories::SqliteItemFieldRepository>(m_database);
    auto itemHistoryRepository = std::make_shared<repositories::SqliteItemHistoryRepository>(m_database);
    auto itemLinkRepository = std::make_shared<repositories::SqliteItemLinkRepository>(m_database);
    auto itemTypeRepository = std::make_shared<repositories::SqliteItemTypeRepository>(m_database);
    auto itemUserStateRepository = std::make_shared<repositories::SqliteItemUserStateRepository>(m_database);
    auto linkTypeRepository = std::make_shared<repositories::SqliteLinkTypeRepository>(m_database);
    auto phaseRepository = std::make_shared<repositories::SqlitePhaseRepository>(m_database);
    auto projectRepository = std::make_shared<repositories::SqliteProjectRepository>(m_database);
    auto roleMenuItemRepository = std::make_shared<repositories::SqliteRoleMenuItemRepository>(m_database);
    auto roleRepository = std::make_shared<repositories::SqliteRoleRepository>(m_database);
    auto ruleItemTypeRepository = std::make_shared<repositories::SqliteRuleItemTypeRepository>(m_database);
    auto ruleRepository = std::make_shared<repositories::SqliteRuleRepository>(m_database);
    auto ruleProjectRepository = std::make_shared<repositories::SqliteRuleProjectRepository>(m_database);
    auto ruleStateRepository = std::make_shared<repositories::SqliteRuleStateRepository>(m_database);
    auto stateRepository = std::make_shared<repositories::SqliteStateRepository>(m_database);
    auto teamRepository = std::make_shared<repositories::SqliteTeamRepository>(m_database);
    auto userRepository = std::make_shared<repositories::SqliteUserRepository>(m_database);
    auto userTeamRoleRepository = std::make_shared<repositories::SqliteUserTeamRoleRepository>(m_database);
    auto workflowRepository = std::make_shared<repositories::SqliteWorkflowRepository>(m_database);


    // === Создаем сервисы ===
    auto authorizationService = std::make_shared<services::AuthorizationService>(
        userRepository,
        ruleRepository,
        ruleProjectRepository,
        ruleItemTypeRepository,
        ruleStateRepository,
        userTeamRoleRepository,
        teamRepository,
        roleRepository
    );
    auto fieldTypeService = std::make_shared<services::FieldTypeService>(
        fieldTypeRepository,
        authorizationService
    );
    auto fieldTypePossibleValueService = std::make_shared<services::FieldTypePossibleValueService>(
        fieldTypePossibleValueRepository,
        fieldTypeRepository,
        authorizationService
    );
    auto itemTypeService = std::make_shared<services::ItemTypeService>(
        itemTypeRepository,
        authorizationService
    );
    auto itemService = std::make_shared<services::ItemService>(
        itemRepository,
        itemFieldRepository,
        itemTypeRepository,
        phaseRepository,
        projectRepository,
        stateRepository,
        fieldTypeRepository,
        authorizationService
    );
    auto itemHistoryService = std::make_shared<services::ItemHistoryService>(
        itemHistoryRepository,
        itemService,
        authorizationService
    );
    auto itemLinkService = std::make_shared<services::ItemLinkService>(
        itemLinkRepository,
        linkTypeRepository,
        phaseRepository,
        itemService,
        authorizationService
    );
    auto itemUserStateService = std::make_shared<services::ItemUserStateService>(
        itemUserStateRepository,
        stateRepository,
        itemService,
        authorizationService
    );
    auto userService = std::make_shared<services::UserService>(
        userRepository,
        authorizationService
    );
    auto linkTypeService = std::make_shared<services::LinkTypeService>(
        linkTypeRepository,
        authorizationService
    );
    auto phaseService = std::make_shared<services::PhaseService>(
        phaseRepository,
        authorizationService
    );
    auto projectService = std::make_shared<services::ProjectService>(
        projectRepository,
        authorizationService
    );
    auto workflowService = std::make_shared<services::WorkflowService>(
        workflowRepository, stateRepository, edgeRepository,
        authorizationService
    );
    auto stateService = std::make_shared<services::StateService>(
        stateRepository, edgeRepository, workflowRepository,
        authorizationService
    );
    auto edgeService = std::make_shared<services::EdgeService>(
        edgeRepository, stateRepository,
        authorizationService
    );
    auto teamService = std::make_shared<services::TeamService>(
        teamRepository,
        userTeamRoleRepository,
        authorizationService
    );
    auto roleService = std::make_shared<services::RoleService>(
        roleRepository,
        authorizationService
    );
    auto ruleService = std::make_shared<services::RuleService>(
        ruleRepository, roleRepository,
        authorizationService
    );
    auto ruleProjectService = std::make_shared<services::RuleProjectService>(
        ruleProjectRepository, ruleRepository, projectRepository,
        authorizationService
    );
    auto ruleItemTypeService = std::make_shared<services::RuleItemTypeService>(
        ruleItemTypeRepository, ruleRepository, itemTypeRepository,
        authorizationService
    );
    auto ruleStateService = std::make_shared<services::RuleStateService>(
        ruleStateRepository, ruleRepository, stateRepository,
        authorizationService
    );
    auto roleMenuItemService = std::make_shared<services::RoleMenuItemService>(
        roleMenuItemRepository, roleRepository,
        authorizationService
    );
    auto userTeamRoleService = std::make_shared<services::UserTeamRoleService>(
        userTeamRoleRepository, userRepository, teamRepository, roleRepository,
        authorizationService
    );
    // TODO: Вынести секретный ключ в конфиг
    auto authMiddleware = std::make_shared<AuthMiddleware>(
        "your-very-long-secret-key-that-is-at-least-32-bytes-long!"
    );
    auto authService = std::make_shared<services::AuthService>(
        userRepository,
        authMiddleware
    );

    // === Создаем REST-сервер ===
    m_restServer = std::make_unique<RestServer>(
        CONFIG.network.apiHost,
        CONFIG.network.apiPort
    );

    m_restServer->setAuthMiddleware(authMiddleware);
    m_restServer->setAuthService(authService);
    m_restServer->setEdgeService(edgeService);
    m_restServer->setFieldTypeService(fieldTypeService);
    m_restServer->setFieldTypePossibleValueService(fieldTypePossibleValueService);
    m_restServer->setItemService(itemService);
    m_restServer->setItemHistoryService(itemHistoryService);
    m_restServer->setItemLinkService(itemLinkService);
    m_restServer->setItemTypeService(itemTypeService);
    m_restServer->setItemUserStateService(itemUserStateService);
    m_restServer->setLinkTypeService(linkTypeService);
    m_restServer->setPhaseService(phaseService);
    m_restServer->setProjectService(projectService);
    m_restServer->setRoleService(roleService);
    m_restServer->setRoleMenuItemService(roleMenuItemService);
    m_restServer->setRuleService(ruleService);
    m_restServer->setRuleItemTypeService(ruleItemTypeService);
    m_restServer->setRuleProjectService(ruleProjectService);
    m_restServer->setRuleStateService(ruleStateService);
    m_restServer->setStateService(stateService);
    m_restServer->setTeamService(teamService);
    m_restServer->setUserService(userService);
    m_restServer->setUserTeamRoleService(userTeamRoleService);
    m_restServer->setWorkflowService(workflowService);

    if (!m_restServer->initialize())
    {
        LOG_ERROR << "Не удалось инициализировать REST-сервер";
        return false;
    }

    LOG_INFO << "Приложение успешно инициализировано";
    return true;
}

int Application::run()
{
    LOG_INFO << "Запуск приложения...";

    if (!initialize())
    {
        LOG_ERROR << "Не удалось инициализировать приложение";
        return EXIT_FAILURE;
    }

    m_isRunning = true;

    if (!m_restServer->start())
    {
        LOG_ERROR << "Не удалось запустить REST-сервер";
        return EXIT_FAILURE;
    }

    LOG_INFO
        << "Приложение запущено на "
        << CONFIG.network.apiHost << ":" << CONFIG.network.apiPort
        << ". Нажмите Ctrl+C для остановки.";

    while (m_isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}

void Application::stop()
{
    LOG_INFO << "Остановка приложения...";
    m_isRunning = false;

    if (m_restServer)
    {
        m_restServer->stop();
    }

    cleanup();
    LOG_INFO << "Приложение остановлено";
}

void Application::cleanup()
{
    if (m_restServer)
    {
        m_restServer.reset();
    }

    if (m_database)
    {
        m_database->shutdown();
        m_database.reset();
    }
}

} // namespace server
