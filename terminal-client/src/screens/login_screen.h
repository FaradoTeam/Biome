#pragma once

#include <functional>
#include <memory>
#include <string>

#include <ftxui/component/screen_interactive.hpp>

#include "../app/navigation_manager.h"
#include "../services/iauth_service.h"

#include "screen.h"

namespace terminal::screens
{

/**
 * @brief Экран входа в систему.
 */
class LoginScreen final : public Screen
{
public:
    using SuccessCallback = std::function<void()>;

    LoginScreen(
        ftxui::ScreenInteractive& screen,
        std::shared_ptr<services::IAuthService> authService,
        std::shared_ptr<NavigationManager> nav,
        SuccessCallback onSuccess
    );

    std::string title() const override
    {
        return "Вход в систему";
    }
    ftxui::Component component() override;

private:
    void doLogin();

private:
    ftxui::ScreenInteractive& m_screen;
    std::shared_ptr<services::IAuthService> m_authService;
    std::shared_ptr<NavigationManager> m_nav;
    SuccessCallback m_onSuccess;

    std::string m_login;
    std::string m_password;
    std::string m_error;
    bool m_loading { false };

    ftxui::Component m_loginInput;
    ftxui::Component m_passwordInput;
    ftxui::Component m_loginButton;
    ftxui::Component m_layout;
    ftxui::Component m_root;
};

} // namespace terminal::screens
