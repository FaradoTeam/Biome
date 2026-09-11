#include <thread>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "common/log/log.h"

#include "main_menu_screen.h"

namespace terminal::screens
{

using namespace ftxui;

MainMenuScreen::MainMenuScreen(
    ftxui::ScreenInteractive& screen,
    std::shared_ptr<services::IAuthService> authService,
    std::shared_ptr<NavigationManager> nav,
    LogoutCallback onLogout,
    ScreenFactory usersScreenFactory
)

    : m_screen(screen)
    , m_authService(std::move(authService))
    , m_nav(std::move(nav))
    , m_onLogout(std::move(onLogout))
    , m_usersScreenFactory(std::move(usersScreenFactory))
    , m_entries({ "Проекты", "Задачи", "Доски", "Команды", "Пользователи", "Сообщения", "Уведомления", "Личные задачи", "Администрирование", "Выйти" })
{
    auto menu = Menu(&m_entries, &m_selected);

    auto menuWithEvent = CatchEvent(
        menu,
        [this](Event event)
        {
            if (event == Event::Return)
            {
                onMenuSelected(m_selected);
                return true;
            }
            if (event == Event::Escape)
            {
                m_nav->quit();
                return true;
            }
            return false;
        }
    );

    m_root = Renderer(
        menuWithEvent,
        [this]() -> Element
        {
            Elements items;
            items.reserve(m_entries.size());

            for (size_t i = 0; i < m_entries.size(); ++i)
            {
                auto entry = text(m_entries[i]);
                if (static_cast<int>(i) == m_selected)
                {
                    entry = entry | bold | color(Color::Yellow);
                }
                else if (m_entries[i] == "Выйти")
                {
                    entry = entry | color(Color::Red);
                }
                items.push_back(entry);
            }

            Elements children = {
                text("Главное меню") | bold | center,
                separator(),
                vbox(std::move(items)),
                separator(),
            };

            if (m_loading)
            {
                children.push_back(
                    text("Выполняется...") | center | color(Color::Yellow)
                );
            }
            else if (!m_status.empty())
            {
                children.push_back(text(m_status) | center);
            }

            children.push_back(
                text("Enter — выбор, Esc — выход") | dim | center
            );

            return vbox(std::move(children))
                | border
                | size(WIDTH, GREATER_THAN, 40)
                | center;
        }
    );
}

Component MainMenuScreen::component()
{
    return m_root;
}

void MainMenuScreen::onMenuSelected(int index)
{
    if (index < 0 || index >= static_cast<int>(m_entries.size()))
        return;

    const std::string& entry = m_entries[index];

    if (entry == "Выйти")
    {
        doLogout();
        return;
    }

    if (entry == "Пользователи")
    {
        if (m_usersScreenFactory)
        {
            m_nav->push(m_usersScreenFactory());
        }
        return;
    }

    // Заглушка: экраны разделов будут добавлены в дальнейшем.
    m_status = "Раздел «" + entry + "» пока не реализован";
}

void MainMenuScreen::doLogout()
{
    if (m_loading)
        return;

    m_loading = true;
    m_status = "Выполняется выход...";

    auto* screen = &m_screen;

    std::thread(
        [this, screen]()
        {
            auto result = m_authService->logout().get();

            screen->Post(
                [this, result]()
                {
                    m_loading = false;

                    if (result.success)
                    {
                        LOG_INFO << "MainMenuScreen: выход выполнен";
                    }
                    else
                    {
                        // Даже если серверная аннулирование упало,
                        // локальный токен уже сброшен — возвращаемся на логин.
                        LOG_WARN
                            << "MainMenuScreen: logout с ошибкой — "
                            << result.errorMessage;
                    }

                    if (m_onLogout)
                    {
                        m_onLogout();
                    }
                }
            );
        }
    ).detach();
}

} // namespace terminal::screens
