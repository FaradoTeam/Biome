#include <cstdlib>
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
    LOG_INFO << "Инициализация приложения...";

    // TODO: Вынести в конфиг secret-key
    auto authMiddleware = std::make_shared<AuthMiddleware>(
        "your-very-long-secret-key-that-is-at-least-32-bytes-long!"
    );

    // TODO: Вынести в конфиг хост и порт
    m_restServer = std::make_unique<RestServer>("0.0.0.0", 8080);
    m_restServer->setAuthMiddleware(authMiddleware);

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

    LOG_INFO << "Приложение запущено. Нажмите Ctrl+C для остановки.";

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
}

} // namespace server
} // namespace farado
