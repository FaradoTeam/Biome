#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include "api/middleware/iauth_middleware.h"

namespace server
{

namespace services
{
class IAuthService;
class IBoardService;
class IBoardColumnService;
class IEdgeService;
class IFieldTypeService;
class IFieldTypePossibleValueService;
class IItemService;
class IItemHistoryService;
class IItemLinkService;
class IItemTypeService;
class IItemUserStateService;
class ILinkTypeService;
class IPhaseService;
class IPlanService;
class IProjectService;
class IProjectTeamService;
class IUserService;
class IStateService;
class IWorkflowService;
class ITeamService;
class IRoleService;
class IRuleService;
class IRuleProjectService;
class IRuleItemTypeService;
class IRuleStateService;
class IRoleMenuItemService;
class IUserTeamRoleService;
}

/**
 * @brief Основной класс REST-сервера.
 *
 * Управляет HTTP-сервером, маршрутизацией запросов, аутентификацией
 * и обработкой запросов к API.
 */
class RestServer final
{
public:
    /**
     * @brief Конструктор REST-сервера.
     *
     * @param host Адрес для привязки ("0.0.0.0" - все интерфейсы)
     * @param port Порт для прослушивания
     */
    explicit RestServer(
        const std::string& host = "0.0.0.0",
        uint16_t port = 8080,
        const std::string& basePath = "/api/v1"
    );
    ~RestServer();

    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;
    RestServer(RestServer&&) = delete;
    RestServer& operator=(RestServer&&) = delete;

    /**
     * @brief Инициализирует сервер: регистрирует маршруты и настраивает
     *        слушатель.
     * @return true при успешной инициализации
     */
    bool initialize();

    /**
     * @brief Запускает сервер и начинает прослушивание входящих соединений.
     * @return true при успешном запуске
     */
    bool start();

    /**
     * @brief Останавливает сервер и закрывает все соединения.
     */
    void stop();

    /**
     * @brief Проверяет, запущен ли сервер.
     * @return true если сервер запущен
     */
    bool isRunning() const { return m_isRunning; }

    void setAuthMiddleware(std::shared_ptr<IAuthMiddleware> middleware);
    void setAuthService(std::shared_ptr<services::IAuthService> authService);
    void setBoardService(std::shared_ptr<services::IBoardService> service);
    void setBoardColumnService(std::shared_ptr<services::IBoardColumnService> service);
    void setFieldTypeService(std::shared_ptr<services::IFieldTypeService> fieldTypeService);
    void setFieldTypePossibleValueService(std::shared_ptr<services::IFieldTypePossibleValueService> service);
    void setItemService(std::shared_ptr<services::IItemService> itemService);
    void setItemHistoryService(std::shared_ptr<services::IItemHistoryService> service);
    void setItemLinkService(std::shared_ptr<services::IItemLinkService> service);
    void setItemTypeService(std::shared_ptr<services::IItemTypeService> itemTypeService);
    void setItemUserStateService(std::shared_ptr<services::IItemUserStateService> service);
    void setLinkTypeService(std::shared_ptr<services::ILinkTypeService> service);
    void setEdgeService(std::shared_ptr<services::IEdgeService> edgeService);
    void setPhaseService(std::shared_ptr<services::IPhaseService> phaseService);
    void setPlanService(std::shared_ptr<services::IPlanService> planService);
    void setProjectService(std::shared_ptr<services::IProjectService> projectService);
    void setProjectTeamService(std::shared_ptr<services::IProjectTeamService> service);
    void setUserService(std::shared_ptr<services::IUserService> userService);
    void setStateService(std::shared_ptr<services::IStateService> stateService);
    void setWorkflowService(std::shared_ptr<services::IWorkflowService> workflowService);
    void setTeamService(std::shared_ptr<services::ITeamService> service);
    void setRoleService(std::shared_ptr<services::IRoleService> service);
    void setRuleService(std::shared_ptr<services::IRuleService> service);
    void setRuleProjectService(std::shared_ptr<services::IRuleProjectService> service);
    void setRuleItemTypeService(std::shared_ptr<services::IRuleItemTypeService> service);
    void setRuleStateService(std::shared_ptr<services::IRuleStateService> service);
    void setRoleMenuItemService(std::shared_ptr<services::IRoleMenuItemService> service);
    void setUserTeamRoleService(std::shared_ptr<services::IUserTeamRoleService> service);

private:
    /**
     * @brief Тип функции-обработчика маршрута.
     *
     * @param request HTTP-запрос
     * @param userId Идентификатор аутентифицированного пользователя
     *               (может быть пустым для публичных маршрутов)
     */
    using RouteHandler = std::function<void(
        const web::http::http_request& request,
        const std::string& userId
    )>;

    /**
     * @brief Структура, описывающая маршрут.
     */
    struct RouteInfo
    {
        web::http::method method; ///< HTTP-метод
        std::string pathPattern; ///< Шаблон пути (регулярное выражение)
        RouteHandler handler; ///< Функция-обработчик
        bool isPublic { false }; ///< Публичный маршрут (без аутентификации)
    };

private:
    /**
     * @brief Регистрирует все маршруты API.
     */
    void registerRoutes();

    /**
     * @brief Настраивает HTTP-слушатель.
     */
    void setupListener();

    /**
     * @brief Основной обработчик входящих запросов.
     *
     * @param request HTTP-запрос
     */
    void handleRequest(web::http::http_request request);

    /**
     * @brief Применяет middleware аутентификации к запросу.
     *
     * @param request HTTP-запрос
     * @param userId Сюда будет записан ID пользователя при успехе
     * @return true если аутентификация пройдена
     */
    bool applyAuthMiddleware(
        const web::http::http_request& request,
        std::string& userId
    );

    /**
     * @brief Добавляет новый маршрут.
     *
     * @param method HTTP-метод (GET, POST, PUT, DELETE)
     * @param path Шаблон пути (может содержать регулярное выражение)
     * @param handler Функция-обработчик
     * @param isPublic Флаг: true - публичный маршрут (без аутентификации)
     */
    void addRoute(
        const web::http::method& method,
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    );

    inline void addRouteGet(
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    )
    {
        addRoute(web::http::methods::GET, path, handler, isPublic);
    }
    inline void addRoutePost(
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    )
    {
        addRoute(web::http::methods::POST, path, handler, isPublic);
    }
    inline void addRoutePut(
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    )
    {
        addRoute(web::http::methods::PUT, path, handler, isPublic);
    }
    inline void addRouteDel(
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    )
    {
        addRoute(web::http::methods::DEL, path, handler, isPublic);
    }

    /**
     * @brief Находит маршрут, соответствующий запросу.
     *
     * @param method HTTP-метод
     * @param uriPath Путь из запроса
     * @param handler Сюда будет записан найденный обработчик
     * @param isPublic Сюда будет записан флаг публичности
     * @param params Сюда будут записаны параметры из пути
     * @return true если маршрут найден
     */
    bool matchRoute(
        const web::http::method& method,
        const std::string& uriPath,
        RouteHandler& handler,
        bool& isPublic,
        std::map<std::string, std::string>& params
    );

private:
    const std::string m_host; ///< Адрес сервера
    const uint16_t m_port; ///< Порт сервера
    const std::string m_basePath; ///< Базовый префикс всех эндпоинтов

private:
    /// HTTP-слушатель
    std::unique_ptr<web::http::experimental::listener::http_listener> m_listener;

    /// Middleware для аутентификации
    std::shared_ptr<IAuthMiddleware> m_authMiddleware;

    /// Флаг состояния сервера
    std::atomic<bool> m_isRunning { false };

    /// Список зарегистрированных маршрутов
    std::vector<RouteInfo> m_routes;

    std::shared_ptr<services::IAuthService> m_authService;
    std::shared_ptr<services::IBoardService> m_boardService;
    std::shared_ptr<services::IBoardColumnService> m_boardColumnService;
    std::shared_ptr<services::IFieldTypeService> m_fieldTypeService;
    std::shared_ptr<services::IFieldTypePossibleValueService> m_fieldTypePossibleValueService;
    std::shared_ptr<services::IItemService> m_itemService;
    std::shared_ptr<services::IItemHistoryService> m_itemHistoryService;
    std::shared_ptr<services::IItemLinkService> m_itemLinkService;
    std::shared_ptr<services::IItemTypeService> m_itemTypeService;
    std::shared_ptr<services::IItemUserStateService> m_itemUserStateService;
    std::shared_ptr<services::ILinkTypeService> m_linkTypeService;
    std::shared_ptr<services::IEdgeService> m_edgeService;
    std::shared_ptr<services::IPhaseService> m_phaseService;
    std::shared_ptr<services::IPlanService> m_planService;
    std::shared_ptr<services::IProjectService> m_projectService;
    std::shared_ptr<services::IProjectTeamService> m_projectTeamService;
    std::shared_ptr<services::IUserService> m_userService;
    std::shared_ptr<services::IStateService> m_stateService;
    std::shared_ptr<services::IWorkflowService> m_workflowService;
    std::shared_ptr<services::ITeamService> m_teamService;
    std::shared_ptr<services::IRoleService> m_roleService;
    std::shared_ptr<services::IRuleService> m_ruleService;
    std::shared_ptr<services::IRuleProjectService> m_ruleProjectService;
    std::shared_ptr<services::IRuleItemTypeService> m_ruleItemTypeService;
    std::shared_ptr<services::IRuleStateService> m_ruleStateService;
    std::shared_ptr<services::IRoleMenuItemService> m_roleMenuItemService;
    std::shared_ptr<services::IUserTeamRoleService> m_userTeamRoleService;
};

} // namespace server
