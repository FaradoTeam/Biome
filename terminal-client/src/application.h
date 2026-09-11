#pragma once

#include <memory>

#include <ftxui/component/screen_interactive.hpp>

#include "api/api_client.h"

#include "app/app_state.h"
#include "app/navigation_manager.h"

#include "services/auth_service.h"
#include "services/user_service.h"

namespace terminal::screens
{
class Screen;
}

namespace terminal
{

class Application final
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

private:
    std::shared_ptr<screens::Screen> createLoginScreen();
    std::shared_ptr<screens::Screen> createMainMenuScreen();
    std::shared_ptr<screens::Screen> createUserListScreen(); // <-- НОВОЕ

private:
    ftxui::ScreenInteractive m_screen;

    std::shared_ptr<AppState> m_appState;
    std::shared_ptr<api::ApiClient> m_apiClient;
    std::shared_ptr<services::IAuthService> m_authService;
    std::shared_ptr<services::IUserService> m_userService; // <-- НОВОЕ
    std::shared_ptr<NavigationManager> m_nav;
};

} // namespace terminal
