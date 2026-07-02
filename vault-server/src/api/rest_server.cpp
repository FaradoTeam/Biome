#include <chrono>
#include <memory>

#include <cpprest/http_msg.h>
#include <cpprest/uri.h>

#include "common/log/log.h"

#include "api/handlers/auth_handler.h"
#include "api/handlers/board_columns_handler.h"
#include "api/handlers/boards_handler.h"
#include "api/handlers/comment_documents_handler.h"
#include "api/handlers/comments_handler.h"
#include "api/handlers/documents_handler.h"
#include "api/handlers/edges_handler.h"
#include "api/handlers/field_type_possible_values_handler.h"
#include "api/handlers/field_types_handler.h"
#include "api/handlers/item_documents_handler.h"
#include "api/handlers/item_histories_handler.h"
#include "api/handlers/item_links_handler.h"
#include "api/handlers/item_types_handler.h"
#include "api/handlers/item_user_states_handler.h"
#include "api/handlers/items_handler.h"
#include "api/handlers/link_types_handler.h"
#include "api/handlers/phases_handler.h"
#include "api/handlers/plans_handler.h"
#include "api/handlers/private_messages_handler.h"
#include "api/handlers/project_teams_handler.h"
#include "api/handlers/projects_handler.h"
#include "api/handlers/role_menu_items_handler.h"
#include "api/handlers/roles_handler.h"
#include "api/handlers/rule_item_types_handler.h"
#include "api/handlers/rule_projects_handler.h"
#include "api/handlers/rule_states_handler.h"
#include "api/handlers/rules_handler.h"
#include "api/handlers/special_days_handler.h"
#include "api/handlers/standard_days_handler.h"
#include "api/handlers/states_handler.h"
#include "api/handlers/team_messages_handler.h"
#include "api/handlers/teams_handler.h"
#include "api/handlers/user_actions_handler.h"
#include "api/handlers/user_days_handler.h"
#include "api/handlers/user_notifications_handler.h"
#include "api/handlers/user_team_roles_handler.h"
#include "api/handlers/user_todos_handler.h"
#include "api/handlers/users_handler.h"
#include "api/handlers/workflows_handler.h"

#include "logic/iauth_service.h"
#include "logic/ifield_type_possible_value_service.h"
#include "logic/ifield_type_service.h"
#include "logic/iitem_service.h"
#include "logic/iitem_type_service.h"
#include "logic/iproject_service.h"
#include "logic/iuser_service.h"

#include "rest_server.h"

namespace server
{

RestServer::RestServer(
    const std::string& host,
    uint16_t port,
    const std::string& basePath
)
    : m_host(host)
    , m_port(port)
    , m_basePath(basePath)
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

void RestServer::setAuthMiddleware(std::shared_ptr<IAuthMiddleware> service)
{
    m_authMiddleware = std::move(service);
}

void RestServer::setAuthService(std::shared_ptr<services::IAuthService> service)
{
    m_authService = std::move(service);
}

void RestServer::setBoardService(std::shared_ptr<services::IBoardService> service)
{
    m_boardService = std::move(service);
}

void RestServer::setBoardColumnService(std::shared_ptr<services::IBoardColumnService> service)
{
    m_boardColumnService = std::move(service);
}

void RestServer::setDocumentService(std::shared_ptr<services::IDocumentService> service)
{
    m_documentService = std::move(service);
}

void RestServer::setCommentService(std::shared_ptr<services::ICommentService> service)
{
    m_commentService = std::move(service);
}

void RestServer::setCommentDocumentService(std::shared_ptr<services::ICommentDocumentService> service)
{
    m_commentDocumentService = std::move(service);
}

void RestServer::setFieldTypeService(std::shared_ptr<services::IFieldTypeService> service)
{
    m_fieldTypeService = std::move(service);
}

void RestServer::setFieldTypePossibleValueService(std::shared_ptr<services::IFieldTypePossibleValueService> service)
{
    m_fieldTypePossibleValueService = std::move(service);
}

void RestServer::setItemDocumentService(std::shared_ptr<services::IItemDocumentService> service)
{
    m_itemDocumentService = std::move(service);
}

void RestServer::setItemService(std::shared_ptr<services::IItemService> service)
{
    m_itemService = std::move(service);
}

void RestServer::setItemHistoryService(std::shared_ptr<services::IItemHistoryService> service)
{
    m_itemHistoryService = std::move(service);
}

void RestServer::setItemLinkService(std::shared_ptr<services::IItemLinkService> service)
{
    m_itemLinkService = std::move(service);
}

void RestServer::setItemTypeService(std::shared_ptr<services::IItemTypeService> service)
{
    m_itemTypeService = std::move(service);
}

void RestServer::setItemUserStateService(std::shared_ptr<services::IItemUserStateService> service)
{
    m_itemUserStateService = std::move(service);
}

void RestServer::setLinkTypeService(std::shared_ptr<services::ILinkTypeService> service)
{
    m_linkTypeService = std::move(service);
}

void RestServer::setEdgeService(std::shared_ptr<services::IEdgeService> service)
{
    m_edgeService = std::move(service);
}

void RestServer::setPhaseService(std::shared_ptr<services::IPhaseService> service)
{
    m_phaseService = std::move(service);
}

void RestServer::setPlanService(std::shared_ptr<services::IPlanService> service)
{
    m_planService = std::move(service);
}

void RestServer::setPrivateMessageService(std::shared_ptr<services::IPrivateMessageService> service)
{
    m_privateMessageService = std::move(service);
}

void RestServer::setProjectService(std::shared_ptr<services::IProjectService> service)
{
    m_projectService = std::move(service);
}

void RestServer::setProjectTeamService(std::shared_ptr<services::IProjectTeamService> service)
{
    m_projectTeamService = std::move(service);
}

void RestServer::setUserService(std::shared_ptr<services::IUserService> service)
{
    m_userService = std::move(service);
}

void RestServer::setStateService(std::shared_ptr<services::IStateService> service)
{
    m_stateService = std::move(service);
}

void RestServer::setSpecialDayService(std::shared_ptr<services::ISpecialDayService> service)
{
    m_specialDayService = std::move(service);
}

void RestServer::setStandardDayService(std::shared_ptr<services::IStandardDayService> service)
{
    m_standardDayService = std::move(service);
}

void RestServer::setWorkflowService(std::shared_ptr<services::IWorkflowService> service)
{
    m_workflowService = std::move(service);
}

void RestServer::setTeamService(std::shared_ptr<services::ITeamService> service)
{
    m_teamService = std::move(service);
}

void RestServer::setTeamMessageService(std::shared_ptr<services::ITeamMessageService> service)
{
    m_teamMessageService = std::move(service);
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

void RestServer::setUserActionService(std::shared_ptr<services::IUserActionService> service)
{
    m_userActionService = std::move(service);
}

void RestServer::setUserDayService(std::shared_ptr<services::IUserDayService> service)
{
    m_userDayService = std::move(service);
}

void RestServer::setUserNotificationService(std::shared_ptr<services::IUserNotificationService> service)
{
    m_userNotificationService = std::move(service);
}

void RestServer::setUserTeamRoleService(std::shared_ptr<services::IUserTeamRoleService> service)
{
    m_userTeamRoleService = std::move(service);
}

void RestServer::setUserTodoService(std::shared_ptr<services::IUserTodoService> service)
{
    m_userTodoService = std::move(service);
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

    // ===== Элементы (Items) =====
    if (m_itemService)
    {
        auto itemsHandler = std::make_shared<handlers::ItemsHandler>(m_itemService);

        // GET /items - список элементов
        addRouteGet(
            "/items",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleGetItems(request, userId);
            }
        );

        // POST /items - создание элемента
        addRoutePost(
            "/items",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleCreateItem(request, userId);
            }
        );

        // GET /items/{id} - получение элемента
        addRouteGet(
            R"(/items/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleGetItem(request, userId);
            }
        );

        // PUT /items/{id} - обновление элемента
        addRoutePut(
            R"(/items/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleUpdateItem(request, userId);
            }
        );

        // DELETE /items/{id} - удаление элемента
        addRouteDel(
            R"(/items/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleDeleteItem(request, userId);
            }
        );

        // POST /items/{id}/restore - восстановление элемента
        addRoutePost(
            R"(/items/(\d+)/restore)",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleRestoreItem(request, userId);
            }
        );

        // GET /items/{id}/fields - получение всех полей элемента
        addRouteGet(
            R"(/items/(\d+)/fields)",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleGetItemFields(request, userId);
            }
        );

        // PUT /items/{id}/fields/{fieldTypeId} - установка поля
        addRoutePut(
            R"(/items/(\d+)/fields/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleSetItemField(request, userId);
            }
        );

        // DELETE /items/{id}/fields/{fieldTypeId} - удаление поля
        addRouteDel(
            R"(/items/(\d+)/fields/(\d+))",
            [itemsHandler](const auto& request, const auto& userId)
            {
                itemsHandler->handleDeleteItemField(request, userId);
            }
        );
    }

    // ===== ItemUserState (история состояний) =====
    if (m_itemUserStateService)
    {
        auto handler = std::make_shared<handlers::ItemUserStatesHandler>(m_itemUserStateService);

        // GET /items/user-states - список с фильтрацией
        addRouteGet(
            "/items/user-states",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemUserStates(request, userId);
            }
        );

        // GET /items/user-states/{id} - получение по ID
        addRouteGet(
            R"(/items/user-states/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemUserState(request, userId);
            }
        );

        // DELETE /items/user-states/{id} - удаление
        addRouteDel(
            R"(/items/user-states/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteItemUserState(request, userId);
            }
        );

        // GET /items/{itemId}/user-states/last - последняя запись
        addRouteGet(
            R"(/items/(\d+)/user-states/last)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetLastItemUserState(request, userId);
            }
        );

        // POST /items/{itemId}/user-states - создание
        addRoutePost(
            R"(/items/(\d+)/user-states)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateItemUserState(request, userId);
            }
        );
    }

    // ===== ItemHistory (история изменений) =====
    if (m_itemHistoryService)
    {
        auto handler = std::make_shared<handlers::ItemHistoriesHandler>(m_itemHistoryService);

        // GET /items/histories - список с фильтрацией
        addRouteGet(
            "/items/histories",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemHistories(request, userId);
            }
        );

        // GET /items/histories/{id} - получение по ID
        addRouteGet(
            R"(/items/histories/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemHistory(request, userId);
            }
        );

        // DELETE /items/histories/{id} - удаление
        addRouteDel(
            R"(/items/histories/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteItemHistory(request, userId);
            }
        );

        // GET /items/{itemId}/histories/last - последняя запись
        addRouteGet(
            R"(/items/(\d+)/histories/last)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetLastItemHistory(request, userId);
            }
        );

        // POST /items/{itemId}/histories - создание
        addRoutePost(
            R"(/items/(\d+)/histories)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateItemHistory(request, userId);
            }
        );
    }

    // ===== Типы связей (LinkTypes) =====
    if (m_linkTypeService)
    {
        auto linkTypesHandler = std::make_shared<handlers::LinkTypesHandler>(m_linkTypeService);

        // GET /link-types - список типов связей
        addRouteGet(
            "/link-types",
            [linkTypesHandler](const auto& request, const auto& userId)
            {
                linkTypesHandler->handleGetLinkTypes(request, userId);
            }
        );

        // POST /link-types - создание типа связи
        addRoutePost(
            "/link-types",
            [linkTypesHandler](const auto& request, const auto& userId)
            {
                linkTypesHandler->handleCreateLinkType(request, userId);
            }
        );

        // GET /link-types/{id} - получение типа связи
        addRouteGet(
            R"(/link-types/(\d+))",
            [linkTypesHandler](const auto& request, const auto& userId)
            {
                linkTypesHandler->handleGetLinkType(request, userId);
            }
        );

        // PUT /link-types/{id} - обновление типа связи
        addRoutePut(
            R"(/link-types/(\d+))",
            [linkTypesHandler](const auto& request, const auto& userId)
            {
                linkTypesHandler->handleUpdateLinkType(request, userId);
            }
        );

        // DELETE /link-types/{id} - удаление типа связи
        addRouteDel(
            R"(/link-types/(\d+))",
            [linkTypesHandler](const auto& request, const auto& userId)
            {
                linkTypesHandler->handleDeleteLinkType(request, userId);
            }
        );
    }

    // ===== Связи элементов (ItemLinks) =====
    if (m_itemLinkService)
    {
        auto itemLinksHandler = std::make_shared<handlers::ItemLinksHandler>(m_itemLinkService);

        // GET /item-links - список связей элементов
        addRouteGet(
            "/item-links",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleGetItemLinks(request, userId);
            }
        );

        // POST /item-links - создание связи
        addRoutePost(
            "/item-links",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleCreateItemLink(request, userId);
            }
        );

        // GET /item-links/{id} - получение связи по ID
        addRouteGet(
            R"(/item-links/(\d+))",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleGetItemLink(request, userId);
            }
        );

        // DELETE /item-links/{id} - удаление связи
        addRouteDel(
            R"(/item-links/(\d+))",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleDeleteItemLink(request, userId);
            }
        );

        // GET /items/{itemId}/links - все связи элемента
        addRouteGet(
            R"(/items/(\d+)/links)",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleGetItemLinksByItemId(request, userId);
            }
        );

        // GET /link-types/{linkTypeId}/links - все связи по типу
        addRouteGet(
            R"(/link-types/(\d+)/links)",
            [itemLinksHandler](const auto& request, const auto& userId)
            {
                itemLinksHandler->handleGetItemLinksByLinkTypeId(request, userId);
            }
        );
    }

    // ===== Планы и элементы планов =====
    if (m_planService)
    {
        auto plansHandler = std::make_shared<handlers::PlansHandler>(m_planService);

        // GET /phases/{phaseId}/plans - список планов фазы
        addRouteGet(
            R"(/phases/(\d+)/plans)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleGetPlansByPhase(request, userId);
            }
        );

        // POST /phases/{phaseId}/plans - создание первого плана в фазе
        addRoutePost(
            R"(/phases/(\d+)/plans)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleCreateFirstPlan(request, userId);
            }
        );

        // GET /plans/{id} - получение плана
        addRouteGet(
            R"(/plans/(\d+))",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleGetPlan(request, userId);
            }
        );

        // DELETE /plans/{id} - удаление плана
        addRouteDel(
            R"(/plans/(\d+))",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleDeletePlan(request, userId);
            }
        );

        // POST /plans/{id}/fork - форк плана
        addRoutePost(
            R"(/plans/(\d+)/fork)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleForkPlan(request, userId);
            }
        );

        // POST /plans/{id}/activate - активация плана
        addRoutePost(
            R"(/plans/(\d+)/activate)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleActivatePlan(request, userId);
            }
        );

        // GET /plans/{planId}/items - получение элементов плана
        addRouteGet(
            R"(/plans/(\d+)/items)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleGetPlanItems(request, userId);
            }
        );

        // POST /plans/{planId}/items - добавление элемента в план
        addRoutePost(
            R"(/plans/(\d+)/items)",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleAddPlanItem(request, userId);
            }
        );

        // GET /plan-items/{id} - получение элемента плана
        addRouteGet(
            R"(/plan-items/(\d+))",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleGetPlanItem(request, userId);
            }
        );

        // PUT /plan-items/{id} - обновление элемента плана
        addRoutePut(
            R"(/plan-items/(\d+))",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleUpdatePlanItem(request, userId);
            }
        );

        // DELETE /plan-items/{id} - удаление элемента плана
        addRouteDel(
            R"(/plan-items/(\d+))",
            [plansHandler](const auto& request, const auto& userId)
            {
                plansHandler->handleDeletePlanItem(request, userId);
            }
        );
    }

    // ===== Доски (Boards) =====
    if (m_boardService)
    {
        auto boardsHandler = std::make_shared<handlers::BoardsHandler>(m_boardService);

        // GET /boards - список досок
        addRouteGet(
            "/boards",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleGetBoards(request, userId);
            }
        );

        // POST /boards - создание доски
        addRoutePost(
            "/boards",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleCreateBoard(request, userId);
            }
        );

        // GET /boards/{id} - получение доски
        addRouteGet(
            R"(/boards/(\d+))",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleGetBoard(request, userId);
            }
        );

        // PUT /boards/{id} - обновление доски
        addRoutePut(
            R"(/boards/(\d+))",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleUpdateBoard(request, userId);
            }
        );

        // DELETE /boards/{id} - удаление доски
        addRouteDel(
            R"(/boards/(\d+))",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleDeleteBoard(request, userId);
            }
        );

        // GET /projects/{projectId}/boards - доски проекта
        addRouteGet(
            R"(/projects/(\d+)/boards)",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleGetBoardsByProject(request, userId);
            }
        );

        // GET /phases/{phaseId}/boards - доски фазы
        addRouteGet(
            R"(/phases/(\d+)/boards)",
            [boardsHandler](const auto& request, const auto& userId)
            {
                boardsHandler->handleGetBoardsByPhase(request, userId);
            }
        );
    }

    // ===== Колонки досок (Board Columns) =====
    if (m_boardColumnService)
    {
        auto boardColumnsHandler = std::make_shared<handlers::BoardColumnsHandler>(m_boardColumnService);

        // GET /board-columns - список колонок
        addRouteGet(
            "/board-columns",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleGetBoardColumns(request, userId);
            }
        );

        // GET /board-columns/{id} - получение колонки
        addRouteGet(
            R"(/board-columns/(\d+))",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleGetBoardColumn(request, userId);
            }
        );

        // PUT /board-columns/{id} - обновление колонки
        addRoutePut(
            R"(/board-columns/(\d+))",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleUpdateBoardColumn(request, userId);
            }
        );

        // DELETE /board-columns/{id} - удаление колонки
        addRouteDel(
            R"(/board-columns/(\d+))",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleDeleteBoardColumn(request, userId);
            }
        );

        // GET /boards/{boardId}/columns - колонки доски
        addRouteGet(
            R"(/boards/(\d+)/columns)",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleGetColumnsByBoard(request, userId);
            }
        );

        // POST /boards/{boardId}/columns - создание колонки
        addRoutePost(
            R"(/boards/(\d+)/columns)",
            [boardColumnsHandler](const auto& request, const auto& userId)
            {
                boardColumnsHandler->handleCreateBoardColumn(request, userId);
            }
        );
    }

    // ===== Пользователи =====
    if (m_userService)
    {
        auto usersHandler = std::make_shared<handlers::UsersHandler>(m_userService);

        addRouteGet(
            "/users",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleGetUsers(request, userId);
            }
        );
        addRoutePost(
            "/users",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleCreateUser(request, userId);
            }
        );
        addRouteGet(
            R"(/users/(\d+))",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleGetUser(request, userId);
            }
        );
        addRoutePut(
            R"(/users/(\d+))",
            [usersHandler](const auto& request, auto& userId)
            {
                usersHandler->handleUpdateUser(request, userId);
            }
        );
        addRouteDel(
            R"(/users/(\d+))",
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
            "/phases",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleGetPhases(request, userId);
            }
        );
        addRoutePost(
            "/phases",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleCreatePhase(request, userId);
            }
        );
        addRouteGet(
            R"(/phases/(\d+))",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleGetPhase(request, userId);
            }
        );
        addRoutePut(
            R"(/phases/(\d+))",
            [phasesHandler](const auto& request, const auto& userId)
            {
                phasesHandler->handleUpdatePhase(request, userId);
            }
        );
        addRouteDel(
            R"(/phases/(\d+))",
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
            "/projects",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleGetProjects(request, userId);
            }
        );

        addRoutePost(
            "/projects",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleCreateProject(request, userId);
            }
        );

        addRouteGet(
            R"(/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleGetProject(request, userId);
            }
        );

        addRoutePut(
            R"(/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleUpdateProject(request, userId);
            }
        );
        addRouteDel(
            R"(/projects/(\d+))",
            [projectsHandler](const auto& request, const auto& userId)
            {
                projectsHandler->handleDeleteProject(request, userId);
            }
        );
    }

    // ===== Project Teams (связи проектов и команд) =====
    if (m_projectTeamService)
    {
        auto handler = std::make_shared<handlers::ProjectTeamsHandler>(m_projectTeamService);

        // GET /project-teams — список связей
        addRouteGet(
            "/project-teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );

        // POST /project-teams — создание связи
        addRoutePost(
            "/project-teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );

        // GET /project-teams/{id} — получение связи по ID
        addRouteGet(
            R"(/project-teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );

        // DELETE /project-teams/{id} — удаление связи
        addRouteDel(
            R"(/project-teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Типы полей =====
    if (m_fieldTypeService)
    {
        auto fieldTypesHandler = std::make_shared<handlers::FieldTypesHandler>(m_fieldTypeService);

        addRoute(
            web::http::methods::GET,
            "/field-types",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleGetFieldTypes(request, userId);
            }
        );
        addRoute(
            web::http::methods::POST,
            "/field-types",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleCreateFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::GET,
            R"(/field-types/(\d+))",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleGetFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::PUT,
            R"(/field-types/(\d+))",
            [fieldTypesHandler](const auto& request, const auto& userId)
            {
                fieldTypesHandler->handleUpdateFieldType(request, userId);
            }
        );
        addRoute(
            web::http::methods::DEL,
            R"(/field-types/(\d+))",
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

        // GET /field-type-values — список с пагинацией
        addRouteGet(
            "/field-type-values",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetValues(request, userId);
            }
        );

        // POST /field-type-values — создание
        addRoutePost(
            "/field-type-values",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateValue(request, userId);
            }
        );

        // GET /field-type-values/{id} — получение по ID
        addRouteGet(
            R"(/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetValue(request, userId);
            }
        );

        // PUT /field-type-values/{id} — обновление
        addRoutePut(
            R"(/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateValue(request, userId);
            }
        );

        // DELETE /field-type-values/{id} — удаление
        addRouteDel(
            R"(/field-type-values/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteValue(request, userId);
            }
        );

        // GET /field-type-values/by-field-type/{fieldTypeId} — значения по типу поля
        addRouteGet(
            R"(/field-type-values/by-field-type/(\d+))",
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
            "/workflows",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleGetWorkflows(request, userId);
            }
        );

        addRoutePost(
            "/workflows",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleCreateWorkflow(request, userId);
            }
        );

        addRouteGet(
            R"(/workflows/(\d+))",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleGetWorkflow(request, userId);
            }
        );

        addRoutePut(
            R"(/workflows/(\d+))",
            [workflowsHandler](auto& request, auto& userId)
            {
                workflowsHandler->handleUpdateWorkflow(request, userId);
            }
        );

        addRouteDel(
            R"(/workflows/(\d+))",
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
            "/item-types",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleGetItemTypes(request, userId);
            }
        );
        addRoute(
            web::http::methods::POST,
            "/item-types",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleCreateItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::GET,
            R"(/item-types/(\d+))",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleGetItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::PUT,
            R"(/item-types/(\d+))",
            [itemTypesHandler](const auto& request, const auto& userId)
            {
                itemTypesHandler->handleUpdateItemType(request, userId);
            }
        );
        addRoute(
            web::http::methods::DEL,
            R"(/item-types/(\d+))",
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
            "/states",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleGetStates(request, userId);
            }
        );

        addRoutePost(
            "/states",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleCreateState(request, userId);
            }
        );

        addRouteGet(
            R"(/states/(\d+))",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleGetState(request, userId);
            }
        );

        addRoutePut(
            R"(/states/(\d+))",
            [statesHandler](auto& request, auto& userId)
            {
                statesHandler->handleUpdateState(request, userId);
            }
        );

        addRouteDel(
            R"(/states/(\d+))",
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
            "/edges",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleGetEdges(request, userId);
            }
        );

        addRoutePost(
            "/edges",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleCreateEdge(request, userId);
            }
        );

        addRouteGet(
            R"(/edges/(\d+))",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleGetEdge(request, userId);
            }
        );

        addRouteDel(
            R"(/edges/(\d+))",
            [edgesHandler](auto& request, auto& userId)
            {
                edgesHandler->handleDeleteEdge(request, userId);
            }
        );

        // Специальный маршрут: получение всех переходов для workflow
        addRouteGet(
            R"(/workflows/(\d+)/edges)",
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
            "/teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetTeams(req, uid);
            }
        );
        addRoutePost(
            "/teams",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateTeam(req, uid);
            }
        );
        addRouteGet(
            R"(/teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetTeam(req, uid);
            }
        );
        addRoutePut(
            R"(/teams/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateTeam(req, uid);
            }
        );
        addRouteDel(
            R"(/teams/(\d+))",
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
            "/roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRoles(req, uid);
            }
        );
        addRoutePost(
            "/roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateRole(req, uid);
            }
        );
        addRouteGet(
            R"(/roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRole(req, uid);
            }
        );
        addRoutePut(
            R"(/roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateRole(req, uid);
            }
        );
        addRouteDel(
            R"(/roles/(\d+))",
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
            "/rules",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRules(req, uid);
            }
        );
        addRoutePost(
            "/rules",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateRule(req, uid);
            }
        );
        addRouteGet(
            R"(/rules/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetRule(req, uid);
            }
        );
        addRoutePut(
            R"(/rules/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateRule(req, uid);
            }
        );
        addRouteDel(
            R"(/rules/(\d+))",
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
            "/rule-projects",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/rule-projects",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/rule-projects/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/rule-projects/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/rule-projects/(\d+))",
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
            "/rule-item-types",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/rule-item-types",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/rule-item-types/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/rule-item-types/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/rule-item-types/(\d+))",
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
            "/rule-states",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/rule-states",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/rule-states/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/rule-states/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/rule-states/(\d+))",
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
            "/role-menu-items",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/role-menu-items",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/role-menu-items/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/role-menu-items/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/role-menu-items/(\d+))",
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
            "/user-team-roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItems(req, uid);
            }
        );
        addRoutePost(
            "/user-team-roles",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleCreateItem(req, uid);
            }
        );
        addRouteGet(
            R"(/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleGetItem(req, uid);
            }
        );
        addRoutePut(
            R"(/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleUpdateItem(req, uid);
            }
        );
        addRouteDel(
            R"(/user-team-roles/(\d+))",
            [handler](const auto& req, const auto& uid)
            {
                handler->handleDeleteItem(req, uid);
            }
        );
    }

    // ===== Private Messages =====
    if (m_privateMessageService)
    {
        auto handler = std::make_shared<handlers::PrivateMessagesHandler>(m_privateMessageService);

        // GET /private-messages - список с фильтрацией и пагинацией
        addRouteGet(
            "/private-messages",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetMessages(request, userId);
            }
        );

        // POST /private-messages - отправка сообщения
        addRoutePost(
            "/private-messages",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleSendMessage(request, userId);
            }
        );

        // GET /private-messages/{id} - получение сообщения
        addRouteGet(
            R"(/private-messages/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetMessage(request, userId);
            }
        );

        // PUT /private-messages/{id}/view - отметка о прочтении
        addRoutePut(
            R"(/private-messages/(\d+)/view)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleMarkAsViewed(request, userId);
            }
        );

        // DELETE /private-messages/{id} - удаление сообщения
        addRouteDel(
            R"(/private-messages/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteMessage(request, userId);
            }
        );

        // GET /private-messages/conversation/{userId} - переписка с пользователем
        addRouteGet(
            R"(/private-messages/conversation/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetConversation(request, userId);
            }
        );

        // GET /private-messages/unviewed/count - количество непрочитанных
        addRouteGet(
            "/private-messages/unviewed/count",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCountUnviewed(request, userId);
            }
        );
    }

    // ===== Team Messages =====
    if (m_teamMessageService)
    {
        auto handler = std::make_shared<handlers::TeamMessagesHandler>(m_teamMessageService);

        // GET /team-messages - список с фильтрацией и пагинацией
        addRouteGet(
            "/team-messages",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetMessages(request, userId);
            }
        );

        // POST /team-messages - отправка сообщения в команду
        addRoutePost(
            "/team-messages",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleSendMessage(request, userId);
            }
        );

        // GET /team-messages/{id} - получение сообщения
        addRouteGet(
            R"(/team-messages/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetMessage(request, userId);
            }
        );

        // DELETE /team-messages/{id} - удаление сообщения
        addRouteDel(
            R"(/team-messages/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteMessage(request, userId);
            }
        );

        // GET /teams/{teamId}/messages - все сообщения команды
        addRouteGet(
            R"(/teams/(\d+)/messages)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetTeamMessages(request, userId);
            }
        );
    }

    // ===== User Notifications =====
    if (m_userNotificationService)
    {
        auto handler = std::make_shared<handlers::UserNotificationsHandler>(m_userNotificationService);

        // GET /user-notifications - список с фильтрацией и пагинацией
        addRouteGet(
            "/user-notifications",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetNotifications(request, userId);
            }
        );

        // POST /user-notifications - подписка на элемент
        addRoutePost(
            "/user-notifications",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleSubscribe(request, userId);
            }
        );

        // GET /user-notifications/{id} - получение подписки
        addRouteGet(
            R"(/user-notifications/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetNotification(request, userId);
            }
        );

        // DELETE /user-notifications/{id} - отписка
        addRouteDel(
            R"(/user-notifications/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUnsubscribe(request, userId);
            }
        );

        // GET /items/{itemId}/subscribers - подписчики элемента
        addRouteGet(
            R"(/items/(\d+)/subscribers)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetSubscribers(request, userId);
            }
        );

        // GET /items/{itemId}/subscribed - проверка подписки
        addRouteGet(
            R"(/items/(\d+)/subscribed)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleIsSubscribed(request, userId);
            }
        );
    }

    // ===== Документы =====
    if (m_documentService)
    {
        auto handler = std::make_shared<handlers::DocumentsHandler>(m_documentService);

        // GET /documents - список документов
        addRouteGet(
            "/documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetDocuments(request, userId);
            }
        );

        // POST /documents - загрузка документа
        addRoutePost(
            "/documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateDocument(request, userId);
            }
        );

        // GET /documents/{id} - получение документа
        addRouteGet(
            R"(/documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetDocument(request, userId);
            }
        );

        // PUT /documents/{id} - обновление метаданных документа
        addRoutePut(
            R"(/documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateDocument(request, userId);
            }
        );

        // DELETE /documents/{id} - удаление документа
        addRouteDel(
            R"(/documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteDocument(request, userId);
            }
        );

        // GET /documents/{id}/download - скачивание документа
        addRouteGet(
            R"(/documents/(\d+)/download)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDownloadDocument(request, userId);
            }
        );
    }

    // ===== Комментарии =====
    if (m_commentService)
    {
        auto handler = std::make_shared<handlers::CommentsHandler>(m_commentService);

        // GET /comments - список комментариев
        addRouteGet(
            "/comments",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetComments(request, userId);
            }
        );

        // POST /comments - создание комментария
        addRoutePost(
            "/comments",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateComment(request, userId);
            }
        );

        // GET /comments/{id} - получение комментария
        addRouteGet(
            R"(/comments/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetComment(request, userId);
            }
        );

        // PUT /comments/{id} - обновление комментария
        addRoutePut(
            R"(/comments/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateComment(request, userId);
            }
        );

        // DELETE /comments/{id} - удаление комментария
        addRouteDel(
            R"(/comments/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteComment(request, userId);
            }
        );

        // GET /items/{itemId}/comments - комментарии элемента
        addRouteGet(
            R"(/items/(\d+)/comments)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetCommentsByItem(request, userId);
            }
        );
    }

    // ===== Связи элементов с документами =====
    if (m_itemDocumentService)
    {
        auto handler = std::make_shared<handlers::ItemDocumentsHandler>(m_itemDocumentService);

        // GET /item-documents - список связей
        addRouteGet(
            "/item-documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemDocuments(request, userId);
            }
        );

        // POST /item-documents - создание связи
        addRoutePost(
            "/item-documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateItemDocument(request, userId);
            }
        );

        // GET /item-documents/{id} - получение связи
        addRouteGet(
            R"(/item-documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemDocument(request, userId);
            }
        );

        // DELETE /item-documents/{id} - удаление связи
        addRouteDel(
            R"(/item-documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteItemDocument(request, userId);
            }
        );

        // GET /items/{itemId}/documents - документы элемента
        addRouteGet(
            R"(/items/(\d+)/documents)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetDocumentsByItem(request, userId);
            }
        );

        // GET /documents/{documentId}/items - элементы документа
        addRouteGet(
            R"(/documents/(\d+)/items)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetItemsByDocument(request, userId);
            }
        );
    }

    // ===== Связи комментариев с документами =====
    if (m_commentDocumentService)
    {
        auto handler = std::make_shared<handlers::CommentDocumentsHandler>(m_commentDocumentService);

        // GET /comment-documents - список связей
        addRouteGet(
            "/comment-documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetCommentDocuments(request, userId);
            }
        );

        // POST /comment-documents - создание связи
        addRoutePost(
            "/comment-documents",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateCommentDocument(request, userId);
            }
        );

        // GET /comment-documents/{id} - получение связи
        addRouteGet(
            R"(/comment-documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetCommentDocument(request, userId);
            }
        );

        // DELETE /comment-documents/{id} - удаление связи
        addRouteDel(
            R"(/comment-documents/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteCommentDocument(request, userId);
            }
        );

        // GET /comments/{commentId}/documents - документы комментария
        addRouteGet(
            R"(/comments/(\d+)/documents)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetDocumentsByComment(request, userId);
            }
        );

        // GET /documents/{documentId}/comments - комментарии документа
        addRouteGet(
            R"(/documents/(\d+)/comments)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetCommentsByDocument(request, userId);
            }
        );
    }

    // ===== Действия пользователей (UserActions) =====
    if (m_userActionService)
    {
        auto userActionsHandler = std::make_shared<handlers::UserActionsHandler>(m_userActionService);

        // GET /user-actions - список действий с пагинацией и фильтрацией
        addRouteGet(
            "/user-actions",
            [userActionsHandler](const auto& request, const auto& userId)
            {
                userActionsHandler->handleGetActions(request, userId);
            }
        );

        // POST /user-actions - создание действия
        addRoutePost(
            "/user-actions",
            [userActionsHandler](const auto& request, const auto& userId)
            {
                userActionsHandler->handleCreateAction(request, userId);
            }
        );

        // GET /user-actions/{id} - получение действия по ID
        addRouteGet(
            R"(/user-actions/(\d+))",
            [userActionsHandler](const auto& request, const auto& userId)
            {
                userActionsHandler->handleGetAction(request, userId);
            }
        );

        // DELETE /user-actions/{id} - удаление действия
        addRouteDel(
            R"(/user-actions/(\d+))",
            [userActionsHandler](const auto& request, const auto& userId)
            {
                userActionsHandler->handleDeleteAction(request, userId);
            }
        );
    }

    // ===== Личные задачи (UserTodos) =====
    if (m_userTodoService)
    {
        auto userTodosHandler = std::make_shared<handlers::UserTodosHandler>(m_userTodoService);

        // GET /user-todos - список задач с пагинацией и фильтрацией
        addRouteGet(
            "/user-todos",
            [userTodosHandler](const auto& request, const auto& userId)
            {
                userTodosHandler->handleGetTodos(request, userId);
            }
        );

        // POST /user-todos - создание задачи
        addRoutePost(
            "/user-todos",
            [userTodosHandler](const auto& request, const auto& userId)
            {
                userTodosHandler->handleCreateTodo(request, userId);
            }
        );

        // GET /user-todos/{id} - получение задачи по ID
        addRouteGet(
            R"(/user-todos/(\d+))",
            [userTodosHandler](const auto& request, const auto& userId)
            {
                userTodosHandler->handleGetTodo(request, userId);
            }
        );

        // PUT /user-todos/{id} - обновление задачи
        addRoutePut(
            R"(/user-todos/(\d+))",
            [userTodosHandler](const auto& request, const auto& userId)
            {
                userTodosHandler->handleUpdateTodo(request, userId);
            }
        );

        // DELETE /user-todos/{id} - удаление задачи
        addRouteDel(
            R"(/user-todos/(\d+))",
            [userTodosHandler](const auto& request, const auto& userId)
            {
                userTodosHandler->handleDeleteTodo(request, userId);
            }
        );
    }

    // ===== Стандартные дни (Standard Days) =====
    if (m_standardDayService)
    {
        auto handler = std::make_shared<handlers::StandardDaysHandler>(m_standardDayService);

        // GET /standard-days - список всех стандартных дней
        addRouteGet(
            "/standard-days",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetStandardDays(request, userId);
            }
        );

        // GET /standard-days/{weekDayNumber} - получение дня по номеру
        addRouteGet(
            R"(/standard-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetStandardDay(request, userId);
            }
        );

        // PUT /standard-days/{weekDayNumber} - обновление дня
        addRoutePut(
            R"(/standard-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateStandardDay(request, userId);
            }
        );
    }

    // ===== Особые дни (Special Days) =====
    if (m_specialDayService)
    {
        auto handler = std::make_shared<handlers::SpecialDaysHandler>(m_specialDayService);

        // GET /special-days - список с пагинацией и фильтрацией
        addRouteGet(
            "/special-days",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetSpecialDays(request, userId);
            }
        );

        // POST /special-days - создание особого дня
        addRoutePost(
            "/special-days",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateSpecialDay(request, userId);
            }
        );

        // GET /special-days/{id} - получение по ID
        addRouteGet(
            R"(/special-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetSpecialDay(request, userId);
            }
        );

        // PUT /special-days/{id} - обновление
        addRoutePut(
            R"(/special-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateSpecialDay(request, userId);
            }
        );

        // DELETE /special-days/{id} - удаление
        addRouteDel(
            R"(/special-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteSpecialDay(request, userId);
            }
        );
    }

    // ===== Пользовательские дни (User Days) =====
    if (m_userDayService)
    {
        auto handler = std::make_shared<handlers::UserDaysHandler>(m_userDayService);

        // GET /user-days - список с пагинацией и фильтрацией
        addRouteGet(
            "/user-days",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetUserDays(request, userId);
            }
        );

        // POST /user-days - создание пользовательского дня
        addRoutePost(
            "/user-days",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleCreateUserDay(request, userId);
            }
        );

        // GET /user-days/{id} - получение по ID
        addRouteGet(
            R"(/user-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetUserDay(request, userId);
            }
        );

        // PUT /user-days/{id} - обновление
        addRoutePut(
            R"(/user-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleUpdateUserDay(request, userId);
            }
        );

        // DELETE /user-days/{id} - удаление
        addRouteDel(
            R"(/user-days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteUserDay(request, userId);
            }
        );

        // GET /users/{userId}/days/{date} - получение дня пользователя по дате
        addRouteGet(
            R"(/users/(\d+)/days/(\d+))",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleGetUserDayByUserAndDate(request, userId);
            }
        );

        // DELETE /users/{userId}/days - удаление всех дней пользователя
        addRouteDel(
            R"(/users/(\d+)/days)",
            [handler](const auto& request, const auto& userId)
            {
                handler->handleDeleteUserDaysByUser(request, userId);
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

    // Регистрируем явный обработчик для OPTIONS
    m_listener->support(
        web::http::methods::OPTIONS,
        [this](web::http::http_request request)
        {
            LOG_INFO << "OPTIONS запрос получен";
            web::http::http_response response(web::http::status_codes::OK);
            response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
            response.headers().add(U("Access-Control-Allow-Methods"), U("GET, POST, PUT, DELETE, OPTIONS"));
            response.headers().add(U("Access-Control-Allow-Headers"), U("Authorization, Content-Type"));
            response.headers().add(U("Access-Control-Max-Age"), U("86400"));
            request.reply(response);
        }
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
    route.pathPattern = m_basePath + path;
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

            // Извлекаем параметры из пути (например, ID из /items/123)
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
