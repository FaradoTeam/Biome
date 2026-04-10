#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace farado
{
namespace server
{

/**
 * @brief Структура, представляющая валидный JWT-токен.
 *
 * Содержит сам токен, идентификатор пользователя и время истечения
 * срока действия.
 */
struct JWTToken
{
    std::string token; ///< Строка JWT-токена
    std::string userId; ///< ID пользователя из токена
    std::chrono::system_clock::time_point expiresAt; ///< Время истечения токена
};

/**
 * @brief Класс для аутентификации запросов через JWT-токены.
 *
 * Предоставляет функциональность для:
 * - Генерации JWT-токенов
 * - Верификации токенов
 * - Аннулирования токенов (черный список)
 * - Извлечения Bearer-токенов из заголовков
 */
class AuthMiddleware final
{
public:
    /**
     * @brief Конструктор.
     * @param secretKey Секретный ключ для подписи JWT-токенов (HS256)
     */
    explicit AuthMiddleware(const std::string& secretKey);
    ~AuthMiddleware() = default;

    AuthMiddleware(const AuthMiddleware&) = delete;
    AuthMiddleware& operator=(const AuthMiddleware&) = delete;
    AuthMiddleware(AuthMiddleware&&) = delete;
    AuthMiddleware& operator=(AuthMiddleware&&) = delete;

    /**
     * @brief Проверяет заголовок Authorization на наличие валидного Bearer-токена.
     *
     * Алгоритм проверки:
     * 1. Извлекает Bearer-токен из заголовка
     * 2. Проверяет, не аннулирован ли токен
     * 3. Верифицирует подпись, срок действия и issuer токена
     *
     * @param authHeader Значение заголовка Authorization
     * @param userId Сюда будет записан user_id из токена в случае успеха
     * @return true если токен присутствует, не аннулирован и прошёл верификацию
     */
    bool validateRequest(const std::string& authHeader, std::string& userId);

    /**
     * @brief Генерирует новый JWT-токен для пользователя.
     *
     * @param userId Идентификатор пользователя
     * @param expiresInSeconds Время жизни токена в секундах
     * @return Строка с JWT-токеном
     */
    std::string generateToken(
        const std::string& userId,
        int expiresInSeconds = 3600
    );

    /**
     * @brief Аннулирует токен (добавляет в чёрный список).
     *
     * Аннулированные токены не могут быть использованы для аутентификации.
     *
     * @param token Токен для аннулирования
     */
    void invalidateToken(const std::string& token);

    /**
     * @brief Проверяет, не был ли токен аннулирован.
     *
     * @param token Токен для проверки
     * @return true если токен находится в черном списке
     */
    bool isTokenInvalidated(const std::string& token);

private:
    /**
     * @brief Извлекает Bearer-токен из заголовка Authorization.
     *
     * Ожидает формат: "Bearer <token>"
     *
     * @param authHeader Значение заголовка Authorization
     * @return Извлеченный токен или пустая строка при ошибке
     */
    std::string extractBearerToken(const std::string& authHeader);

    /**
     * @brief Верифицирует JWT-токен.
     *
     * Проверяет подпись, срок действия и issuer.
     *
     * @param token JWT-токен для верификации
     * @return JWTToken при успехе, std::nullopt при ошибке
     */
    std::optional<JWTToken> verifyToken(const std::string& token);

    /**
     * @brief Очищает черный список от истекших токенов.
     *
     * Удаляет токены, срок действия которых уже истек,
     * чтобы предотвратить неограниченный рост черного списка.
     */
    void cleanBlacklist();

private:
    /// Секретный ключ для подписи JWT
    std::string m_secretKey;

    /// Черный список аннулированных токенов
    std::unordered_map<std::string, std::chrono::system_clock::time_point> m_blacklist;

    /// Мьютекс для потокобезопасного доступа к черному списку
    std::mutex m_blacklistMutex;
};

} // namespace server
} // namespace farado
