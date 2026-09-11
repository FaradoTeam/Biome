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

    /// Вызывается при возврате на экран после pop() — можно перезагрузить данные.
    virtual void onResume() { }
};

} // namespace terminal::screens