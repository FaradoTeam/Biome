#pragma once

#include <memory>
#include <vector>

#include <ftxui/component/screen_interactive.hpp>

namespace terminal::screens
{
class Screen;
}

namespace terminal
{

/**
 * @brief Управляет стеком экранов и переключением между ними.
 *
 * Использует простой стек: push/pop/replace. После изменения стека
 * вызывается ScreenInteractive::Exit(), чтобы внешний цикл Application
 * перезапустил Loop с новым экраном.
 */
class NavigationManager final
{
public:
    explicit NavigationManager(ftxui::ScreenInteractive& screen);

    void push(std::shared_ptr<screens::Screen> screen);
    void pop();
    void replace(std::shared_ptr<screens::Screen> screen);

    /// Полный выход из приложения.
    void quit();

    bool isActive() const { return !m_shouldQuit && !m_stack.empty(); }
    std::shared_ptr<screens::Screen> current() const;

private:
    ftxui::ScreenInteractive& m_screen;
    std::vector<std::shared_ptr<screens::Screen>> m_stack;
    bool m_shouldQuit { false };
};

} // namespace terminal
