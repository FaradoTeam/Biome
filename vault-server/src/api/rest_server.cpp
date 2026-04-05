#include <iostream>

#include <memory>
#include <nlohmann/json.hpp>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "common/log/log.h"

#include "rest_server.h"

#include "handlers/auth_handler.h"
#include "handlers/items_handler.h"

namespace farado
{
namespace server
{

RestServer::RestServer(const std::string& host, uint16_t port)
    : m_server(std::make_unique<httplib::Server>())
    , m_host(host)
    , m_port(port)
    , m_isRunning(false)
{
    LOG_INFO << "RestServer created with host=" << m_host << ", port=" << m_port;
}

RestServer::~RestServer()
{
    stop();
}

bool RestServer::initialize()
{
    LOG_INFO << "Initializing REST server...";

    if (!m_server)
    {
        LOG_ERROR << "Failed to create HTTP server";
        return false;
    }

    // Настройка сервера
    m_server->set_keep_alive_max_count(100);
    m_server->set_read_timeout(std::chrono::seconds(30));
    m_server->set_write_timeout(std::chrono::seconds(30));
    m_server->set_idle_interval(std::chrono::seconds(5));

    // Регистрируем обработчики ошибок
    m_server->set_error_handler(
        [](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json error;
            error["code"] = res.status;
            error["message"] = "Internal server error";
            error["path"] = req.path;

            res.set_content(error.dump(), "application/json");
        }
    );

    m_server->set_exception_handler(
        [](const httplib::Request& req, httplib::Response& res, std::exception_ptr ep)
        {
            nlohmann::json error;
            error["code"] = 500;
            error["message"] = "Internal server exception";
            error["path"] = req.path;

            res.status = 500;
            res.set_content(error.dump(), "application/json");
        }
    );

    // Регистрируем маршруты
    registerRoutes();

    LOG_INFO << "REST server initialized successfully";
    return true;
}

void RestServer::registerRoutes()
{
    // Создаем обработчики
    auto itemsHandler = std::make_shared<handlers::ItemsHandler>();
    auto authHandler = std::make_shared<handlers::AuthHandler>(m_authMiddleware);

    // =====================================================
    // Публичные маршруты (без аутентификации)
    // =====================================================

    // Аутентификация
    post(
        "/auth/login",
        [authHandler](const httplib::Request& req, httplib::Response& res)
        {
            authHandler->handleLogin(req, res);
        }
    );

    post(
        "/auth/logout",
        [authHandler](const httplib::Request& req, httplib::Response& res)
        {
            authHandler->handleLogout(req, res);
        }
    );

    // =====================================================
    // Защищенные маршруты (требуют аутентификации)
    // =====================================================

    // Items API
    get(
        "/api/items",
        [itemsHandler, this](const httplib::Request& req, httplib::Response& res)
        {
            std::string userId;
            if (!applyAuthMiddleware(req, res, userId))
            {
                return;
            }
            itemsHandler->handleGetItems(req, res, userId);
        }
    );

    post(
        "/api/items",
        [itemsHandler, this](const httplib::Request& req, httplib::Response& res)
        {
            std::string userId;
            if (!applyAuthMiddleware(req, res, userId))
            {
                return;
            }
            itemsHandler->handleCreateItem(req, res, userId);
        }
    );

    get(
        R"(/api/items/(\d+))",
        [itemsHandler, this](const httplib::Request& req, httplib::Response& res)
        {
            std::string userId;
            if (!applyAuthMiddleware(req, res, userId))
            {
                return;
            }
            itemsHandler->handleGetItem(req, res, userId);
        }
    );

    put(
        R"(/api/items/(\d+))",
        [itemsHandler, this](const httplib::Request& req, httplib::Response& res)
        {
            std::string userId;
            if (!applyAuthMiddleware(req, res, userId))
            {
                return;
            }
            itemsHandler->handleUpdateItem(req, res, userId);
        }
    );

    del(
        R"(/api/items/(\d+))",
        [itemsHandler, this](const httplib::Request& req, httplib::Response& res)
        {
            std::string userId;
            if (!applyAuthMiddleware(req, res, userId))
            {
                return;
            }
            itemsHandler->handleDeleteItem(req, res, userId);
        }
    );

    // Health check (публичный)
    get(
        "/health",
        [](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json status;
            status["status"] = "ok";
            status["timestamp"] = std::time(nullptr);
            res.set_content(status.dump(), "application/json");
        }
    );

    LOG_INFO << "Routes registered successfully";
}

bool RestServer::applyAuthMiddleware(
    const httplib::Request& req,
    httplib::Response& res,
    std::string& userId
)
{
    if (!m_authMiddleware)
    {
        LOG_ERROR << "Auth middleware not set";
        res.status = 500;
        nlohmann::json error;
        error["code"] = 500;
        error["message"] = "Internal server error: auth middleware not configured";
        res.set_content(error.dump(), "application/json");
        return false;
    }

    if (!m_authMiddleware->validateRequest(req, userId))
    {
        res.status = 401;
        nlohmann::json error;
        error["code"] = 401;
        error["message"] = "Unauthorized: invalid or missing token";
        res.set_content(error.dump(), "application/json");
        return false;
    }

    return true;
}

void RestServer::get(const std::string& path, RouteHandler handler)
{
    m_server->Get(path, handler);
}

void RestServer::post(const std::string& path, RouteHandler handler)
{
    m_server->Post(path, handler);
}

void RestServer::put(const std::string& path, RouteHandler handler)
{
    m_server->Put(path, handler);
}

void RestServer::del(const std::string& path, RouteHandler handler)
{
    m_server->Delete(path, handler);
}

void RestServer::setAuthMiddleware(std::shared_ptr<AuthMiddleware> middleware)
{
    m_authMiddleware = middleware;
    LOG_INFO << "Auth middleware set";
}

bool RestServer::start()
{
    if (m_isRunning)
    {
        LOG_ERROR << "REST server is already running";
        return false;
    }

    LOG_INFO << "Starting REST server on " << m_host << ":" << m_port << "...";

    m_isRunning = true;

    // Запускаем сервер в отдельном потоке
    m_serverThread = std::make_unique<std::thread>(
        [this]()
        {
            if (!m_server->listen(m_host.c_str(), m_port))
            {
                LOG_ERROR << "Failed to start server on " << m_host << ":" << m_port;
                m_isRunning = false;
            }
        }
    );

    // Даем серверу время на запуск
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (m_isRunning)
    {
        LOG_INFO << "REST server started successfully on " << m_host << ":" << m_port;
    }

    return m_isRunning;
}

void RestServer::stop()
{
    if (!m_isRunning)
    {
        return;
    }

    LOG_INFO << "Stopping REST server...";
    m_isRunning = false;

    if (m_server)
    {
        m_server->stop();
    }

    if (m_serverThread && m_serverThread->joinable())
    {
        m_serverThread->join();
    }

    LOG_INFO << "REST server stopped";
}

} // namespace server
} // namespace farado
