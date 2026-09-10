#pragma once

#include <string>

namespace terminal
{

/**
 * @brief Глобальное состояние приложения.
 *
 * Хранит токен аутентификации, флаг входа. В дальнейшем сюда будут
 * добавляться текущий пользователь, кэш проектов, контекст навигации и т.д.
 */
class AppState final
{
public:
    bool isLoggedIn() const
    {
        return m_loggedIn;
    }
    void setLoggedIn(bool value)
    {
        m_loggedIn = value;
    }

    const std::string& token() const
    {
        return m_token;
    }
    void setToken(std::string token)
    {
        m_token = std::move(token);
    }

    /// Полный сброс состояния при выходе из системы.
    void logout()
    {
        m_loggedIn = false;
        m_token.clear();
    }

private:
    bool m_loggedIn { false };
    std::string m_token;
};

} // namespace terminal