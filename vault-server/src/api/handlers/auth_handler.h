#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "../middleware/auth_middleware.h"

namespace farado
{
namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов аутентификации.
 *
 * Предоставляет методы для входа в систему, выхода и смены пароля.
 * Работает в связке с AuthMiddleware для генерации и проверки JWT-токенов.
 */
class AuthHandler final
{
public:
    /**
     * @brief Конструктор обработчика аутентификации.
     * @param authMiddleware Указатель на middleware для работы с JWT-токенами
     */
    explicit AuthHandler(std::shared_ptr<AuthMiddleware> authMiddleware);

    /**
     * @brief Обрабатывает запрос на вход пользователя.
     *
     * Ожидает JSON-тело запроса с полями "login" и "password".
     * При успешной аутентификации возвращает JWT-токен.
     *
     * @param request HTTP-запрос от клиента
     */
    void handleLogin(const web::http::http_request& request);

    /**
     * @brief Обрабатывает запрос на выход пользователя.
     *
     * Извлекает Bearer-токен из заголовка Authorization и аннулирует его.
     *
     * @param request HTTP-запрос от клиента
     */
    void handleLogout(const web::http::http_request& request);

    /**
     * @brief Обрабатывает запрос на смену пароля.
     *
     * Ожидает JSON-тело с полями "oldPassword" и "newPassword".
     * Требует валидной аутентификации пользователя.
     *
     * @param request HTTP-запрос от клиента
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleChangePassword(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    /**
     * @brief Проверяет учетные данные пользователя.
     *
     * @param login Логин пользователя
     * @param password Пароль пользователя
     * @return Идентификатор пользователя при успехе, пустая строка при ошибке
     */
    std::string validateCredentials(
        const std::string& login,
        const std::string& password
    );

    /**
     * @brief Отправляет ошибку в формате JSON.
     *
     * @param response HTTP-ответ для модификации
     * @param code Код ошибки
     * @param message Текст сообщения об ошибке
     */
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

private:
    /// Middleware для работы с JWT
    std::shared_ptr<AuthMiddleware> m_authMiddleware;
};

} // namespace handlers
} // namespace server
} // namespace farado
