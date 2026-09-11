#include "../screens/screen.h"

#include "navigation_manager.h"

namespace terminal
{

NavigationManager::NavigationManager(ftxui::ScreenInteractive& screen)
    : m_screen(screen)
{
}

void NavigationManager::push(std::shared_ptr<screens::Screen> screen)
{
    m_stack.push_back(std::move(screen));
    m_screen.Exit();
}

void NavigationManager::pop()
{
    if (m_stack.size() > 1)
    {
        m_stack.pop_back();

        if (auto screen = current())
        {
            screen->onResume();
        }
    }
    m_screen.Exit();
}

void NavigationManager::replace(std::shared_ptr<screens::Screen> screen)
{
    if (!m_stack.empty())
    {
        m_stack.pop_back();
    }
    m_stack.push_back(std::move(screen));
    m_screen.Exit();
}

void NavigationManager::quit()
{
    m_shouldQuit = true;
    m_screen.Exit();
}

std::shared_ptr<screens::Screen> NavigationManager::current() const
{
    return m_stack.empty() ? nullptr : m_stack.back();
}

} // namespace terminal
