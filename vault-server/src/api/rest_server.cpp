#include <chrono>
#include <memory>

#include <cpprest/http_msg.h>
#include <cpprest/uri.h>

#include "common/log/log.h"

#include "api/handlers/auth_handler.h"
#include "api/handlers/edges_handler.h"
#include "api/handlers/field_type_possible_values_handler.h"
#include "api/handlers/field_types_handler.h"
#include "api/handlers/item_types_handler.h"
#include "api/handlers/items_handler.h"
#include "api/handlers/phases_handler.h"
#include "api/handlers/projects_handler.h"
#include "api/handlers/role_menu_items_handler.h"
#include "api/handlers/roles_handler.h"
#include "api/handlers/rule_item_types_handler.h"
#include "api/handlers/rule_projects_handler.h"
#include "api/handlers/rule_states_handler.h"
#include "api/handlers/rules_handler.h"
#include "api/handlers/states_handler.h"
#include "api/handlers/teams_handler.h"
#include "api/handlers/user_team_roles_handler.h"
#include "api/handlers/users_handler.h"
#include "api/handlers/workflows_handler.h"

#include "logic/iauth_service.h"
#include "logic/ifield_type_possible_value_service.h"
#include "logic/ifield_type_service.h"
#include "logic/iitem_type_service.h"
#include "logic/iproject_service.h"
#include "logic/iuser_service.h"

#include "rest_server.h"

namespace server
{

RestServer::RestServer(const std::string& host, uint16_t port)
    : m_host(host)
    , m_port(port)
    , m_isRunning(false)
{
    LOG_DEBUG
        << "REST-сервера создан. Хост=" << m_host << ", порт=" << m_port << ".";
}

RestServer::~RestServer()
{
    stop(); // Гарантируем остановку сервера при разрушении объекта
}

bool RestServer::initialize()
{
    LOG_DEBUG << "Инициализация REST-сервера...";

    registerRoutes(); // Регистрируем все маршруты API
    setupListener(); // Настраиваем HTTP-слушатель

    LOG_DEBUG << "REST-сервера успешно инициализирован";
    return true;
}

bool RestServer::start()
{
    if (m_isRunning)
    {
        LOG_ERROR << "Сервер REST уже запущен";
        return false;
    }

    try
    {
        m_listener->open().wait(); // Открываем слушатель и ждем готовности
        m_isRunning = true;
        LOG_INFO
            << "Сервер REST успешно запущен на "
            << m_host << ":" << m_port;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Не удалось запустить сервер: " << e.what();
        return false;
    }

    return true;
}

void RestServer::stop()
{
    if (!m_isRunning)
    {
        return;
    }

    LOG_DEBUG << "Остановка REST-сервера...";
    m_isRunning = false;

    try
    {
        if (m_listener)
        {
            m_listener->close().wait(); // Закрываем слушатель и ждем завершения
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при остановке REST-сервера: " << e.what();
    }

    LOG_INFO << "Сервер REST остановлен";
}

void RestServer::setAuthMiddleware(std::shared_ptr<IAuthMiddleware> middleware)
{
    m_authMiddleware = middleware;
}

void RestServer::setAuthService(std::shared_ptr<services::IAuthService> authService)
{
    m_authService = authService;
}

void RestServer::setFieldTypeService(std::shared_ptr<services::IFieldTypeService> fieldTypeService)
{
    m_fieldTypeService = fieldTypeService;
}

void RestServer::setFieldTypePossibleValueService(std::shared_ptr<services::IFieldTypePossibleValueService> service)
{
    m_fieldTypePossibleValueService = service;
}

void RestServer::setItemTypeService(std::shared_ptr<services::IItemTypeService> itemTypeService)
{
    m_itemTypeService = itemTypeService;
}

void RestServer::setEdgeService(std::shared_ptr<services::IEdgeService> edgeService)
{
    m_edgeService = edgeService;
}

void RestServer::setPhaseService(std::shared_ptr<services::IPhaseService> phaseService)
{
    m_phaseService = phaseService;
}

void RestServer::setProjectService(std::shared_ptr<services::IProjectService> projectService)
{
    m_projectService = projectService;
}

void RestServer::setUserService(std::shared_ptr<services::IUserService> userService)
{
    m_userService = userService;
}

void RestServer::setStateService(std::shared_ptr<services::IStateService> stateService)
{
    m_stateService = stateService;
}

void RestServer::setWorkflowService(std::shared_ptr<services::IWorkflowService> workflowService)
{
    m_workflowService = workflowService;
}

void RestServer::setTeamService(std::shared_ptr<services::ITeamService> service)
{
    m_teamService = std::move(service);
}

void RestServer::setRoleService(std::shared_ptr<services::IRoleService> service)
{
    m_roleService = std::move(service);
}

void RestServer::setRuleService(std::shared_ptr<services::IRuleService> service)
{
    m_ruleService = std::move(service);
}

void RestServer::setRuleProjectService(std::shared_ptr<services::IRuleProjectService> service)
{
    m_ruleProjectService = std::move(service);
}

void RestServer::setRuleItemTypeService(std::shared_ptr<services::IRuleItemTypeService> service)
{
    m_ruleItemTypeService = std::move(service);
}

void RestServer::setRuleStateService(std::shared_ptr<services::IRuleStateService> service)
{
    m_ruleStateService = std::move(service);
}

void RestServer::setRoleMenuItemService(std::shared_ptr<services::IRoleMenuItemService> service)
{
    m_roleMenuItemService = std::move(service);
}

void RestServer::setUserTeamRoleService(std::shared_ptr<services::IUserTeamRoleService> service)
{
    m_userTeamRoleService = std::move(service);
}

void RestServer::registerRoutes()
{
    // ===== Аутентификация =====
    if (m_authService)
    {
        auto authHandler = std::make_shared<handlers::AuthHandler>(m_authService);

        addRoutePost(
            "/auth/login",
            [authHandler](const auto& request, const auto& /*userId*/)
            {
                authHandler->handleLogin(request);
            },
            true
        );
        addRoutePost(
            "/auth/logout",
            [authHandler](const auto& request, const auto& /*userId*/)
            {
                authHandler->handleLogout(request);
            },
            true
        );
        addRoutePost(
            "/auth/change-password",
            [authHandler](const auto& request, const auto& userId)
            {
                authHandler->handleChangePassword(request, userId);
            },
            false // требуется аутентификация
        );
    }

    // ===== Элементы =====
    {
        auto itemsHandler = std::make_shared<handlers::ItemsHandler>();

        addRouteGet(
            "/api/items",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleGetItems(request, userId);
            }
        );
        addRoutePost(
            "/api/items",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleCreateItem(request, userId);
            }
        );
        addRouteGet(
            R"(/api/items/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleGetItem(request, userId);
            },
            false
        );
        addRoutePut(
            R"(/api/items/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleUpdateItem(request, userId);
            },
            false
        );
        addRouteDel(
            R"(/api/items/(\d+))",
            [itemsHandler](const auto& request, auto& userId)
            {
                itemsHandler->handleDeleteItem(request, userId);
            },
            false
        );
    }

    // ===== Пользователи =====
    if (m_userService)
    {
        auto usersHandler = std::make_shared<handlers::UsersHandler>(m_userService);

        addRouteGet(
            "/api/users",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleGetUsers(request, userId);
            }
        );
        addRoutePost(
            "/api/users",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleCreateUser(request, userId);
            }
        );
        addRouteGet(
            R"(/api/users/(\d+))",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleGetUser(request, userId);
            }
        );
        addRoutePut(
            R"(/api/users/(\d+))",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleUpdateUser(request, userId);
            }
        );
        addRouteDel(
            R"(/api/users/(\d+))",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleDeleteUser(request, userId);
            }
        );
    }

    // ===== Фазы =====
    if (m_phaseService)
    {
        auto phasesHandler = std::make_shared<handlers::PhasesHandler>(m_phaseService);

        addRouteGet(
            "/api/phases",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleGetPhases(request, userId);
            }
        );
        addRoutePost(
            "/api/phases",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleCreatePhase(request, userId);
            }
        );
        addRouteGet(
            R"(/api/phases/(\d+))",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleGetPhase(request, userId);
            }
        );
        addRoutePut(
            R"(/api/phases/(\d+))",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleUpdatePhase(request, userId);
            }
        );
        addRouteDel(
            R"(/api/phases/(\d+))",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleDeletePhase(request, userId);
            }
        );
    }

    // ===== Проекты =====
    if (m_projectService)
    {
        auto projectsHandler = std::make_shared<handlers::ProjectsHandler>(m_projectService);

        addRouteGet(
            "/api/projects",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleGetProjects(request, userId);
            }
        );

        addRoutePost(
            "/api/projects",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleCreateProject(request, userId);
            }
        );

        addRouteGet(
            R"(/api/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleGetProject(request, userId);
            }
        );

        addRoutePut(
            R"(/api/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleUpdateProject(request, userId);
            }
        );
        addRouteDel(
            R"(/api/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleDeleteProject(request, userId);
            }
        );
    }

    // ===== Типы полей =====
    if (m_fieldTypeService)
    {
        auto fieldTypesHandler = std::make_shared<handlers::FieldTypesHandler>(m_fieldTypeService);

        addRoute(
            web::http::methods::GET,
            "/api/field-types",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleGetFieldTypes(request, userId);
            }
        );
        addRoute(
            web::http::methods::POST,
            "/api/field-types",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleCreateFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::GET,
            R"(/api/field-types/(\d+))",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleGetFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::PUT,
            R"(/api/field-types/(\d+))",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleUpdateFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::DEL,
            R"(/api/field-types/(\d+))",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleDeleteFieldType(request, userId);
            }
        );
    }

    // ===== Возможные значения типов полей =====
    if (m_fieldTypePossibleValueService)
    {
        auto handler = std::make_shared<handlers::FieldTypePossibleValuesHandler>(
            m_fieldTypePossibleValueService
        );

        // GET /api/field-type-values — список с пагинацией
        addRouteGet(
            "/api/field-type-values",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetValues(request, userId);
            }
        );

        // POST /api/field-type-values — создание
        addRoutePost(
            "/api/field-type-values",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateValue(request, userId);
            }
        );

        // GET /api/field-type-values/{id} — получение по ID
        addRouteGet(
            R"(/api/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetValue(request, userId);
            }
        );

        // PUT /api/field-type-values/{id} — обновление
        addRoutePut(
            R"(/api/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateValue(request, userId);
            }
        );

        // DELETE /api/field-type-values/{id} — удаление
        addRouteDel(
            R"(/api/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteValue(request, userId);
            }
        );

        // GET /api/field-type-values/by-field-type/{fieldTypeId} — значения по типу поля
        addRouteGet(
            R"(/api/field-type-values/by-field-type/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetValuesByFieldType(request, userId);
            }
        );
    }

    // ===== Рабочие процессы =====
    if (m_workflowService)
    {
        auto workflowsHandler = std::make_shared<handlers::WorkflowsHandler>(m_workflowService);

        addRouteGet(
            "/api/workflows",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleGetWorkflows(request, userId);
            }
        );

        addRoutePost(
            "/api/workflows",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleCreateWorkflow(request, userId);
            }
        );

        addRouteGet(
            R"(/api/workflows/(\d+))",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleGetWorkflow(request, userId);
            }
        );

        addRoutePut(
            R"(/api/workflows/(\d+))",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleUpdateWorkflow(request, userId);
            }
        );

        addRouteDel(
            R"(/api/workflows/(\d+))",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleDeleteWorkflow(request, userId);
            }
        );
    }

    // ===== Типы элементов =====
    if (m_itemTypeService)
    {
        auto itemTypesHandler = std::make_shared<handlers::ItemTypesHandler>(m_itemTypeService);

        addRoute(
            web::http::methods::GET,
            "/api/item-types",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleGetItemTypes(request, userId);
            }
        );
        addRoute(
            web::http::methods::POST,
            "/api/item-types",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleCreateItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::GET,
            R"(/api/item-types/(\d+))",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleGetItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::PUT,
            R"(/api/item-types/(\d+))",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleUpdateItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::DEL,
            R"(/api/item-types/(\d+))",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleDeleteItemType(request, userId);
            }
        );
    }

    // ===== Состояния =====
    if (m_stateService)
    {
        auto statesHandler = std::make_shared<handlers::StatesHandler>(m_stateService);

        addRouteGet(
            "/api/states",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleGetStates(request, userId);
            }
        );

        addRoutePost(
            "/api/states",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleCreateState(request, userId);
            }
        );

        addRouteGet(
            R"(/api/states/(\d+))",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleGetState(request, userId);
            }
        );

        addRoutePut(
            R"(/api/states/(\d+))",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleUpdateState(request, userId);
            }
        );

        addRouteDel(
            R"(/api/states/(\d+))",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleDeleteState(request, userId);
            }
        );
    }

    // ===== Переходы =====
    if (m_edgeService)
    {
        auto edgesHandler = std::make_shared<handlers::EdgesHandler>(m_edgeService);

        addRouteGet(
            "/api/edges",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleGetEdges(request, userId);
            }
        );

        addRoutePost(
            "/api/edges",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleCreateEdge(request, userId);
            }
        );

        addRouteGet(
            R"(/api/edges/(\d+))",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleGetEdge(request, userId);
            }
        );

        addRouteDel(
            R"(/api/edges/(\d+))",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleDeleteEdge(request, userId);
            }
        );

        // Специальный маршрут: получение всех переходов для workflow
        addRouteGet(
            R"(/api/workflows/(\d+)/edges)",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleGetWorkflowEdges(request, userId);
            }
        );
    }

    // ===== Teams =====
    if (m_teamService)
    {
        auto handler = std::make_shared<handlers::TeamsHandler>(m_teamService);
        addRouteGet(
            "/api/teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetTeams(req, uid);
            }
        );
        addRoutePost(
            "/api/teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateTeam(req, uid);
            }
        );
        addRouteGet(
            R"(/api/teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetTeam(req, uid);
            }
        );
        addRoutePut(
            R"(/api/teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateTeam(req, uid);
            }
        );
        addRouteDel(
            R"(/api/teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteTeam(req, uid);
            }
        );
    }

    // ===== Roles =====
    if (m_roleService)
    {
        auto handler = std::make_shared<handlers::RolesHandler>(m_roleService);
        addRouteGet(
            "/api/roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRoles(req, uid);
            }
        );
        addRoutePost(
            "/api/roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateRole(req, uid);
            }
        );
        addRouteGet(
            R"(/api/roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRole(req, uid);
            }
        );
        addRoutePut(
            R"(/api/roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateRole(req, uid);
            }
        );
        addRouteDel(
            R"(/api/roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteRole(req, uid);
            }
        );
    }

    // ===== Rules =====
    if (m_ruleService)
    {
        auto handler = std::make_shared<handlers::RulesHandler>(m_ruleService);
        addRouteGet(
            "/api/rules",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRules(req, uid);
            }
        );
        addRoutePost(
            "/api/rules",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateRule(req, uid);
            }
        );
        addRouteGet(
            R"(/api/rules/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRule(req, uid);
            }
        );
        addRoutePut(
            R"(/api/rules/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateRule(req, uid);
            }
        );
        addRouteDel(
            R"(/api/rules/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteRule(req, uid);
            }
        );
    }

    // ===== Rule Projects =====
    if (m_ruleProjectService)
    {
        auto handler = std::make_shared<handlers::RuleProjectsHandler>(m_ruleProjectService);
        addRouteGet(
            "/api/rule-projects",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/api/rule-projects",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/api/rule-projects/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/api/rule-projects/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/api/rule-projects/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Rule Item Types =====
    if (m_ruleItemTypeService)
    {
        auto handler = std::make_shared<handlers::RuleItemTypesHandler>(m_ruleItemTypeService);
        addRouteGet(
            "/api/rule-item-types",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/api/rule-item-types",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/api/rule-item-types/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/api/rule-item-types/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/api/rule-item-types/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Rule States =====
    if (m_ruleStateService)
    {
        auto handler = std::make_shared<handlers::RuleStatesHandler>(m_ruleStateService);
        addRouteGet(
            "/api/rule-states",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/api/rule-states",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/api/rule-states/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/api/rule-states/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/api/rule-states/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Role Menu Items =====
    if (m_roleMenuItemService)
    {
        auto handler = std::make_shared<handlers::RoleMenuItemsHandler>(m_roleMenuItemService);
        addRouteGet(
            "/api/role-menu-items",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/api/role-menu-items",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/api/role-menu-items/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/api/role-menu-items/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/api/role-menu-items/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== User Team Roles =====
    if (m_userTeamRoleService)
    {
        auto handler = std::make_shared<handlers::UserTeamRolesHandler>(m_userTeamRoleService);
        addRouteGet(
            "/api/user-team-roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/api/user-team-roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/api/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/api/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/api/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Работоспособность сервера (Health check) =====
    {
        addRouteGet(
            "/health",
            [](const auto& request, const auto& /*userId*/)
            {
                const auto now = std::chrono::system_clock::now().time_since_epoch();
                web::json::value response;
                response["status"] = web::json::value::string("ok");
                response["timestamp"] = web::json::value::number(
                    std::chrono::duration_cast<std::chrono::seconds>(now).count()
                );
                request.reply(web::http::status_codes::OK, response);
            },
            true // публичный эндпоинт
        );
    }

    LOG_DEBUG
        << "Успешно зарегистрированные маршруты, всего: " << m_routes.size();
}

void RestServer::setupListener()
{
    const web::http::uri address = web::http::uri(
        "http://" + m_host + ":" + std::to_string(m_port)
    );
    m_listener = std::make_unique<web::http::experimental::listener::http_listener>(
        address
    );

    // Устанавливаем универсальный обработчик всех запросов
    m_listener->support(
        [this](web::http::http_request request)
        {
            try
            {
                handleRequest(std::move(request));
            }
            catch (const std::exception& e)
            {
                // Глобальный обработчик ошибок — любое неперехваченное исключение
                // приводит к ответу 500 Internal Server Error
                LOG_ERROR << "Ошибка обработки запроса: " << e.what();
                web::json::value error;
                error["code"] = web::json::value::number(500);
                error["message"] = web::json::value::string(
                    "Internal server error: " + std::string(e.what())
                );
                request.reply(web::http::status_codes::InternalError, error);
            }
        }
    );
}

void RestServer::handleRequest(web::http::http_request request)
{
    const std::string path = web::uri::decode(request.relative_uri().path());
    const auto method = request.method();

    LOG_INFO << "Входящий запрос: " << method << " " << path;

    RouteHandler handler;
    bool isPublic = false;
    std::map<std::string, std::string> pathParams;

    // Ищем обработчик для данного маршрута
    if (!matchRoute(method, path, handler, isPublic, pathParams))
    {
        // Маршрут не найден — возвращаем 404
        web::json::value error;
        error["code"] = web::json::value::number(404);
        error["message"] = web::json::value::string("Not found");
        request.reply(web::http::status_codes::NotFound, error);
        return;
    }

    // Применяем аутентификацию только для защищенных маршрутов
    if (!isPublic)
    {
        std::string userId;
        if (!applyAuthMiddleware(request, userId))
        {
            return; // Ответ уже отправлен в applyAuthMiddleware
        }
        handler(request, userId); // Вызываем обработчик с ID пользователя
    }
    else
    {
        handler(request, ""); // Публичный маршрут — userId не требуется
    }
}

bool RestServer::applyAuthMiddleware(
    const web::http::http_request& request,
    std::string& userId
)
{
    // Проверяем наличие middleware
    if (!m_authMiddleware)
    {
        LOG_ERROR << "Auth middleware не установлено";
        web::json::value error;
        error["code"] = web::json::value::number(500);
        error["message"] = web::json::value::string(
            "Internal server error: auth middleware not configured"
        );
        request.reply(web::http::status_codes::InternalError, error);
        return false;
    }

    // Ищем заголовок Authorization
    auto authHeader = request.headers().find("Authorization");
    if (authHeader == request.headers().end())
    {
        web::json::value error;
        error["code"] = web::json::value::number(401);
        error["message"] = web::json::value::string(
            "Missing Authorization header"
        );
        request.reply(web::http::status_codes::Unauthorized, error);
        return false;
    }

    // Валидируем токен
    if (!m_authMiddleware->validateRequest(authHeader->second, userId))
    {
        web::json::value error;
        error["code"] = web::json::value::number(401);
        error["message"] = web::json::value::string(
            "Invalid or expired token"
        );
        request.reply(web::http::status_codes::Unauthorized, error);
        return false;
    }

    return true; // Аутентификация успешна
}

void RestServer::addRoute(
    const web::http::method& method,
    const std::string& path,
    RouteHandler handler,
    bool isPublic
)
{
    RouteInfo route;
    route.method = method;
    route.pathPattern = path;
    route.handler = handler;
    route.isPublic = isPublic;
    m_routes.push_back(route);
}

bool RestServer::matchRoute(
    const web::http::method& method,
    const std::string& uriPath,
    RouteHandler& handler,
    bool& isPublic,
    std::map<std::string, std::string>& params
)
{
    for (const auto& route : m_routes)
    {
        // Проверяем соответствие HTTP-метода
        if (route.method != method)
            continue;

        // Проверяем соответствие пути с помощью регулярного выражения
        std::regex pattern(route.pathPattern);
        std::smatch matches;

        if (std::regex_match(uriPath, matches, pattern))
        {
            handler = route.handler;
            isPublic = route.isPublic;

            // Извлекаем параметры из пути (например, ID из /api/items/123)
            if (matches.size() > 1)
            {
                params["id"] = matches[1].str();
            }
            return true;
        }
    }
    return false; // Маршрут не найден
}

} // namespace server
