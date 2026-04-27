#include <cstdlib>

#include <nlohmann/json.hpp>

#include "common/log/log.h"

#include "config.h"
#include "application.h"

#include "api/middleware/impl/auth_middleware.h"
#include "api/rest_server.h"

#include "logic/impl/auth_service.h"
#include "logic/impl/user_service.h"

#include "repo/sqlite/sqlite_user_repository.h"

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

    // 1. Инициализируем базу данных
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

    // 2. Создаем репозиторий пользователей
    m_userRepository = std::make_shared<repositories::SqliteUserRepository>(m_database);
    m_userService = std::make_shared<services::UserService>(m_userRepository);

    // 3. Создаем middleware для аутентификации
    // TODO: Вынести секретный ключ в конфиг
    m_authMiddleware = std::make_shared<AuthMiddleware>(
        "your-very-long-secret-key-that-is-at-least-32-bytes-long!"
    );

    // 4. Создаем сервис аутентификации
    m_authService = std::make_shared<services::AuthService>(
        m_userRepository,
        m_authMiddleware
    );

    // 5. Создаем REST-сервер
    m_restServer = std::make_unique<RestServer>(
        CONFIG.network.apiHost,
        CONFIG.network.apiPort
    );

    m_restServer->setAuthMiddleware(m_authMiddleware);
    m_restServer->setAuthService(m_authService);
    m_restServer->setUserService(m_userService);

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

    m_userRepository.reset();
    m_authService.reset();
    m_authMiddleware.reset();
}

} // namespace server
