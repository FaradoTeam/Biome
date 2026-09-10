#include "common/log/log.h"

#include "screens/login_screen.h"
#include "screens/main_menu_screen.h"

#include "application.h"

namespace terminal
{

Application::Application()
    : m_screen(ftxui::ScreenInteractive::Fullscreen())
{
    m_appState = std::make_shared<AppState>();
    m_apiClient = std::make_shared<api::ApiClient>(
        "http://localhost:8090" // TODO: брать из конфига/CLI.
    );
    m_authService = std::make_shared<services::AuthService>(
        m_apiClient, m_appState
    );
    m_nav = std::make_shared<NavigationManager>(m_screen);

    LOG_INFO << "Application создан";
}

Application::~Application()
{
    LOG_INFO << "Application уничтожен";
}

int Application::run()
{
    LOG_INFO << "Запуск приложения";

    // Начальный экран.
    m_nav->replace(createLoginScreen());

    // Основной цикл: прогоняем текущий экран, пока стек не пуст
    // и пользователь не запросил выход.
    while (m_nav->isActive())
    {
        auto screen = m_nav->current();
        if (!screen)
        {
            break;
        }

        m_screen.Loop(screen->component());
    }

    LOG_INFO << "Приложение завершено";
    return 0;
}

std::shared_ptr<screens::Screen> Application::createLoginScreen()
{
    return std::make_shared<screens::LoginScreen>(
        m_screen,
        m_authService,
        m_nav,
        [this]()
        {
            // Вызывается из UI-потока после успешного входа.
            m_nav->replace(createMainMenuScreen());
        }
    );
}

std::shared_ptr<screens::Screen> Application::createMainMenuScreen()
{
    return std::make_shared<screens::MainMenuScreen>(
        m_screen,
        m_authService,
        m_nav,
        [this]()
        {
            // Вызывается из UI-потока после выхода из системы.
            m_nav->replace(createLoginScreen());
        }
    );
}

} // namespace terminal
