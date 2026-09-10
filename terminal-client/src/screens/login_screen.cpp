#include <ftxui/dom/elements.hpp>

#include "common/log/log.h"

#include "login_screen.h"

namespace terminal::screens
{

using namespace ftxui;

LoginScreen::LoginScreen(
    ftxui::ScreenInteractive& screen,
    std::shared_ptr<services::IAuthService> authService,
    std::shared_ptr<NavigationManager> nav,
    SuccessCallback onSuccess
)
    : m_screen(screen)
    , m_authService(std::move(authService))
    , m_nav(std::move(nav))
    , m_onSuccess(std::move(onSuccess))
{
    m_loginInput = Input(&m_login, "Логин или email");
    m_passwordInput = Input(&m_password, "Пароль");
    m_passwordInput |= CatchEvent(
        [this](Event event)
        {
            if (event == Event::Return)
            {
                doLogin();
                return true;
            }
            return false;
        }
    );

    m_loginButton = Button(
        "Войти",
        [this]
        { doLogin(); },
        ButtonOption::Ascii()
    );

    m_layout = Container::Vertical({
        m_loginInput,
        m_passwordInput,
        m_loginButton,
    });

    auto renderer = Renderer(
        m_layout,
        [this]() -> Element
        {
            Elements children = {
                text("Вход в систему") | bold | center,
                separator(),
                hbox({ text("Логин:  "), m_loginInput->Render() | flex }),
                hbox({ text("Пароль: "), m_passwordInput->Render() | flex }),
                separator(),
                m_loginButton->Render() | center,
            };

            if (m_loading)
            {
                children.push_back(
                    text("Выполняется вход...") | center | color(Color::Yellow)
                );
            }
            else if (!m_error.empty())
            {
                children.push_back(
                    text(m_error) | color(Color::Red) | center
                );
            }

            children.push_back(separator());
            children.push_back(
                text("Tab — переход, Enter — вход, Esc — выход")
                | dim | center
            );

            return vbox(std::move(children))
                | border
                | size(WIDTH, GREATER_THAN, 50)
                | center;
        }
    );

    // Обработка Escape для выхода из приложения.
    m_root = CatchEvent(
        renderer,
        [this](Event event)
        {
            if (event == Event::Escape)
            {
                m_nav->quit();
                return true;
            }
            return false;
        }
    );
}

Component LoginScreen::component()
{
    return m_root;
}

void LoginScreen::doLogin()
{
    if (m_loading)
        return;

    if (m_login.empty() || m_password.empty())
    {
        m_error = "Введите логин и пароль";
        return;
    }

    m_loading = true;
    m_error.clear();

    // Захватываем данные для фонового потока.
    const auto login = m_login;
    const auto password = m_password;
    auto* screen = &m_screen;

    std::thread(
        [this, screen, login, password]()
        {
            auto result = m_authService->login(login, password).get();

            // Возвращаемся в UI-поток через ScreenInteractive::Post.
            screen->Post(
                [this, result]()
                {
                    m_loading = false;

                    if (result.success)
                    {
                        LOG_INFO << "LoginScreen: успешный вход";
                        if (m_onSuccess)
                        {
                            m_onSuccess(); // Application выполнит replace.
                        }
                    }
                    else
                    {
                        m_error = "Ошибка: " + result.errorMessage;
                    }
                }
            );
        }
    ).detach();
}

} // namespace terminal::screens
