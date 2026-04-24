#include <chrono>
#include <regex>

#include <nlohmann/json.hpp>

#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>

#include "common/log/log.h"

#include "auth_middleware.h"

namespace server
{

AuthMiddleware::AuthMiddleware(const std::string& secretKey)
    : m_secretKey(secretKey)
{
    if (m_secretKey.empty())
    {
        // Пустой секретный ключ — серьезная проблема безопасности!
        LOG_ERROR << "AuthMiddleware инициализируется пустым секретным ключом";
    }
}

bool AuthMiddleware::validateRequest(
    const std::string& authHeader,
    std::string& userId
)
{
    // 1. Проверяем наличие заголовка Authorization
    if (authHeader.empty())
    {
        LOG_ERROR << "Отсутствует заголовок авторизации";
        return false;
    }

    // 2. Извлекаем Bearer-токен
    const std::string token = extractBearerToken(authHeader);
    if (token.empty())
    {
        LOG_ERROR << "Недопустимый формат заголовка авторизации";
        return false;
    }

    // 3. Проверяем, не аннулирован ли токен
    if (isTokenInvalidated(token))
    {
        LOG_ERROR << "Токен был признан недействительным";
        return false;
    }

    // 4. Верифицируем подпись, срок действия и issuer
    const auto jwtToken = verifyToken(token);
    if (!jwtToken.has_value())
    {
        LOG_ERROR << "Недействительный или просроченный токен";
        return false;
    }

    // 5. Всё успешно — возвращаем userId из токена
    userId = jwtToken->userId;
    return true;
}

std::string AuthMiddleware::generateToken(
    const std::string& userId,
    int expiresInSeconds
)
{
    const auto now = std::chrono::system_clock::now();
    const auto expiresAt = now + std::chrono::seconds(expiresInSeconds);

    LOG_DEBUG
        << "Генерация токена для пользователя " << userId
        << " истекает: " << std::chrono::system_clock::to_time_t(expiresAt);

    // Создаем JWT-токен с необходимыми claims
    return jwt::create()
        // Указываем тип токена
        .set_type("JWT")
        // Указываем издателя
        .set_issuer("biome-api")
        // Пользовательский claim с ID пользователя
        .set_payload_claim("user_id", jwt::claim(userId))
        // Время выдачи
        .set_issued_at(now)
        // Время истечения
        .set_expires_at(expiresAt)
        // Подписываем HMAC-SHA256
        .sign(jwt::algorithm::hs256 { m_secretKey });
}

void AuthMiddleware::invalidateToken(const std::string& token)
{
    // Сначала проверяем, что токен валиден
    auto jwtToken = verifyToken(token);
    if (!jwtToken.has_value())
    {
        LOG_ERROR << "Невозможно аннулировать недействительный токен";
        return;
    }

    LOG_DEBUG
        << "Токен проверен, пользователь: " << jwtToken->userId << ", истекает: "
        << std::chrono::system_clock::to_time_t(jwtToken->expiresAt);

    // Добавляем токен в черный список вместе с временем истечения
    {
        std::lock_guard<std::mutex> lock(m_blacklistMutex);
        m_blacklist[token] = jwtToken->expiresAt;
        LOG_DEBUG
            << "Токен добавлен в черный список, размер черного списка: "
            << m_blacklist.size();
    }

    // Периодически очищаем черный список от истекших токенов
    // TODO: сделать фоном в отдельном потоке, а не при invalidateToken
    static int invalidateCount = 0;
    if (++invalidateCount % 10 == 0) // Каждые 10 аннулирований
        cleanBlacklist();
}

bool AuthMiddleware::isTokenInvalidated(const std::string& token)
{
    std::lock_guard<std::mutex> lock(m_blacklistMutex);

    auto it = m_blacklist.find(token);
    if (it == m_blacklist.end())
        return false; // Токен не в черном списке

    // Проверяем, не истек ли уже срок действия токена
    const auto now = std::chrono::system_clock::now();
    if (it->second < now)
    {
        // Токен истек — удаляем из черного списка
        // (он больше не представляет угрозы)
        m_blacklist.erase(it);
        return false;
    }

    return true; // Токен в черном списке и еще не истек
}

std::string AuthMiddleware::extractBearerToken(const std::string& authHeader)
{
    // Регулярное выражение для формата "Bearer <token>"
    // Токен может содержать буквы, цифры, дефис, подчеркивание и точку
    std::regex bearerRegex(R"(^Bearer\s+([a-zA-Z0-9\-_\.]+)$)");
    std::smatch matches;

    if (std::regex_match(authHeader, matches, bearerRegex) && matches.size() > 1)
    {
        return matches[1].str(); // Возвращаем извлеченный токен
    }

    return ""; // Неверный формат
}

std::optional<JWTToken> AuthMiddleware::verifyToken(const std::string& token)
{
    try
    {
        // Декодируем токен (без проверки подписи)
        auto decoded = jwt::decode(token);

        // Настраиваем верификатор
        auto verifier = jwt::verify()
                            // Разрешаем только HS256
                            .allow_algorithm(jwt::algorithm::hs256 { m_secretKey })
                            // Проверяем issuer
                            .with_issuer("biome-api");

        // Выполняем верификацию (подпись, срок действия, issuer)
        verifier.verify(decoded);

        // Токен валиден — извлекаем информацию
        JWTToken result;
        result.token = token;
        result.userId = decoded.get_payload_claim("user_id").as_string();
        result.expiresAt = decoded.get_expires_at();
        return result;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Не удалось выполнить проверку токена: " << e.what();
    }
    return std::nullopt; // Верификация не удалась
}

void AuthMiddleware::cleanBlacklist()
{
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(m_blacklistMutex);

    // Удаляем все токены, срок действия которых истек
    std::erase_if(
        m_blacklist,
        [now](const auto& item)
        {
            return item.second < now;
        }
    );
}

} // namespace server
