#pragma once

#include <ftxui/component/component.hpp>

#include <string>

namespace terminal::screens
{

/**
 * @brief Базовый интерфейс экрана.
 */
class Screen
{
public:
    virtual ~Screen() = default;

    /// Заголовок (используется для отладки и, потенциально, для UI).
    virtual std::string title() const = 0;

    /// Возвращает ftxui-компонент для отображения.
    virtual ftxui::Component component() = 0;
};

} // namespace terminal::screens