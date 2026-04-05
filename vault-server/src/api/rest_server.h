#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "middleware/auth_middleware.h"

namespace httplib
{
class Server;
class Request;
class Response;
}

namespace farado
{
namespace server
{

// class AuthMiddleware;

class RestServer final
{
public:
    using RouteHandler = std::function<void(const httplib::Request&, httplib::Response&)>;

    explicit RestServer(const std::string& host = "0.0.0.0", uint16_t port = 8080);
    ~RestServer();

    bool initialize();
    bool start();
    void stop();
    bool isRunning() const { return m_isRunning; }

    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);

    void setAuthMiddleware(std::shared_ptr<AuthMiddleware> middleware);

private:
    void registerRoutes();
    bool applyAuthMiddleware(
        const httplib::Request& req,
        httplib::Response& res,
        std::string& userId
    );

private:
    const std::string m_host;
    const uint16_t m_port;

    std::unique_ptr<httplib::Server> m_server;
    std::shared_ptr<AuthMiddleware> m_authMiddleware;
    std::atomic<bool> m_isRunning { false };
    std::unique_ptr<std::thread> m_serverThread;
};

} // namespace server
} // namespace farado
