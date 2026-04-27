#include <chrono>
#include <memory>
#include <regex>

#include <cpprest/http_msg.h>
#include <cpprest/uri.h>

#include "common/log/log.h"

#include "api/handlers/auth_handler.h"
#include "api/handlers/items_handler.h"
#include "api/handlers/users_handler.h"

#include "logic/iauth_service.h"
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

void RestServer::setUserService(std::shared_ptr<services::IUserService> userService)
{
    m_userService = userService;
}

void RestServer::registerRoutes()
{
    if (!m_authService
        || !m_authMiddleware
        || !m_userService)
    {
        LOG_ERROR
            << "Ошибка создания обработчиков для REST-сервера! "
            << "Нет необходимых экземпляров сервисов бизнес логики.";
        return;
    }

    // Создаем обработчики
    auto itemsHandler = std::make_shared<handlers::ItemsHandler>();
    auto authHandler = std::make_shared<handlers::AuthHandler>(
        m_authService,
        m_authMiddleware
    );
    auto usersHandler = std::make_shared<handlers::UsersHandler>(m_userService);

    // Публичные маршруты
    addRoute(
        web::http::methods::POST,
        "/auth/login",
        [authHandler](
            const web::http::http_request& request,
            const std::string& /*userId*/
        )
        {
            authHandler->handleLogin(request);
        },
        true
    );

    addRoute(
        web::http::methods::POST,
        "/auth/logout",
        [authHandler](
            const web::http::http_request& request,
            const std::string& /*userId*/
        )
        {
            authHandler->handleLogout(request);
        },
        true
    );

    // Защищенный маршрут для смены пароля
    addRoute(
        web::http::methods::POST,
        "/auth/change-password",
        [authHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            authHandler->handleChangePassword(request, userId);
        },
        false // требуется аутентификация
    );

    // Защищенные маршруты (требуют аутентификации)

    // Получение списка элементов
    addRoute(
        web::http::methods::GET,
        "/api/items",
        [itemsHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            itemsHandler->handleGetItems(request, userId);
        }
    );

    // Создание нового элемента
    addRoute(
        web::http::methods::POST,
        "/api/items",
        [itemsHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            itemsHandler->handleCreateItem(request, userId);
        }
    );

    // Получение элемента по ID (шаблон с параметром)
    addRoute(
        web::http::methods::GET,
        R"(/api/items/(\d+))", // Регулярное выражение для извлечения ID
        [itemsHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            itemsHandler->handleGetItem(request, userId);
        },
        false
    );

    // Обновление элемента
    addRoute(
        web::http::methods::PUT,
        R"(/api/items/(\d+))",
        [itemsHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            itemsHandler->handleUpdateItem(request, userId);
        },
        false
    );

    // Удаление элемента
    addRoute(
        web::http::methods::DEL,
        R"(/api/items/(\d+))",
        [itemsHandler](
            const web::http::http_request& request,
            const std::string& userId
        )
        {
            itemsHandler->handleDeleteItem(request, userId);
        },
        false
    );

    // Эндпоинт для проверки работоспособности сервера (health check)
    addRoute(
        web::http::methods::GET,
        "/health",
        [](
            const web::http::http_request& request,
            const std::string& /*userId*/
        )
        {
            web::json::value response;
            response["status"] = web::json::value::string("ok");
            response["timestamp"] = web::json::value::number(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                )
                    .count()
            );
            request.reply(web::http::status_codes::OK, response);
        },
        true // публичный эндпоинт
    );

    addRoute(
        web::http::methods::GET,
        "/api/users",
        [usersHandler](auto& req, auto& userId)
        {
            usersHandler->handleGetUsers(req, userId);
        }
    );
    addRoute(
        web::http::methods::POST,
        "/api/users",
        [usersHandler](auto& req, auto& userId)
        {
            usersHandler->handleCreateUser(req, userId);
        }
    );
    addRoute(
        web::http::methods::GET,
        R"(/api/users/(\d+))",
        [usersHandler](auto& req, auto& userId)
        {
            usersHandler->handleGetUser(req, userId);
        }
    );
    addRoute(
        web::http::methods::PUT,
        R"(/api/users/(\d+))",
        [usersHandler](auto& req, auto& userId)
        {
            usersHandler->handleUpdateUser(req, userId);
        }
    );
    addRoute(
        web::http::methods::DEL,
        R"(/api/users/(\d+))",
        [usersHandler](auto& req, auto& userId)
        {
            usersHandler->handleDeleteUser(req, userId);
        }
    );

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
