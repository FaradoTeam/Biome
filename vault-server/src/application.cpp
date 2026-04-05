#include <cstdlib>
#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include "common/log/log.h"

#include "application.h"

#include "api/rest_server.h"


namespace farado
{
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
    LOG_INFO << "Initializing application...";

    auto authMiddleware = std::make_shared<AuthMiddleware>(
        "secret-key-here-from-config"
    );

    m_restServer = std::make_shared<RestServer>("0.0.0.0", 8080);
    m_restServer->setAuthMiddleware(authMiddleware);
    
    if (!m_restServer->initialize())
    {
        LOG_ERROR << "Failed to initialize REST server";
        return false;
    }

    LOG_INFO << "Application initialized successfully";
    return true;
}

int Application::run()
{
    LOG_INFO << "Starting application...";

    if (!initialize())
    {
        LOG_ERROR << "Failed to initialize application";
        return EXIT_FAILURE;
    }

    m_isRunning = true;

    if (!m_restServer->start())
    {
        LOG_ERROR << "Failed to start REST server";
        return EXIT_FAILURE;
    }

    LOG_INFO << "Application is running. Press Ctrl+C to stop.";

    while (m_isRunning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}

void Application::stop()
{
    LOG_INFO << "Stopping application...";
    m_isRunning = false;

    if (m_restServer)
    {
        m_restServer->stop();
    }

    cleanup();
    LOG_INFO << "Application stopped";
}

void Application::cleanup()
{
    if (m_restServer)
    {
        m_restServer.reset();
    }
}

} // namespace server
} // namespace farado
