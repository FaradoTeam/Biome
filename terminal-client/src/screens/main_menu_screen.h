#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/screen_interactive.hpp>

#include "../app/navigation_manager.h"
#include "../services/iauth_service.h"

#include "screen.h"

namespace terminal::screens
{

class MainMenuScreen final : public Screen
{
public:
    using LogoutCallback = std::function<void()>;
    using ScreenFactory = std::function<std::shared_ptr<Screen>()>;

    MainMenuScreen(
        ftxui::ScreenInteractive& screen,
        std::shared_ptr<services::IAuthService> authService,
        std::shared_ptr<NavigationManager> nav,
        LogoutCallback onLogout,
        ScreenFactory usersScreenFactory // <-- НОВОЕ
    );

    std::string title() const override { return "Главное меню"; }
    ftxui::Component component() override;

private:
    void onMenuSelected(int index);
    void doLogout();

private:
    ftxui::ScreenInteractive& m_screen;
    std::shared_ptr<services::IAuthService> m_authService;
    std::shared_ptr<NavigationManager> m_nav;
    LogoutCallback m_onLogout;
    ScreenFactory m_usersScreenFactory;

    std::vector<std::string> m_entries;
    int m_selected { 0 };
    bool m_loading { false };
    std::string m_status;

    ftxui::Component m_root;
};

} // namespace terminal::screens
