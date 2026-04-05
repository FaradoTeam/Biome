#include <iostream>
#include <regex>

#include <httplib.h>

#include <jwt-cpp/jwt.h>

#include "auth_middleware.h"

namespace farado
{
namespace server
{

AuthMiddleware::AuthMiddleware(const std::string& secretKey)
    : m_secretKey(secretKey)
{
    if (m_secretKey.empty())
    {
        std::cerr << "Warning: AuthMiddleware initialized with empty secret key" << std::endl;
    }
}

bool AuthMiddleware::validateRequest(const httplib::Request& req, std::string& userId)
{
    auto authHeader = req.get_header_value("Authorization");
    if (authHeader.empty())
    {
        std::cerr << "Missing Authorization header" << std::endl;
        return false;
    }

    std::string token = extractBearerToken(authHeader);
    if (token.empty())
    {
        std::cerr << "Invalid Authorization header format" << std::endl;
        return false;
    }

    if (isTokenInvalidated(token))
    {
        std::cerr << "Token has been invalidated" << std::endl;
        return false;
    }

    auto jwtToken = verifyToken(token);
    if (!jwtToken.has_value())
    {
        std::cerr << "Invalid or expired token" << std::endl;
        return false;
    }

    userId = jwtToken->userId;
    return true;
}

std::string AuthMiddleware::generateToken(const std::string& userId, int expiresInSeconds)
{
    const auto now = std::chrono::system_clock::now();
    const auto expiresAt = now + std::chrono::seconds(expiresInSeconds);

    return jwt::create()
        .set_type("JWT")
        .set_issuer("farado-api")
        .set_payload_claim("user_id", jwt::claim(userId))
        .set_issued_at(now)
        .set_expires_at(expiresAt)
        .sign(jwt::algorithm::hs256 { m_secretKey });
}

std::optional<JWTToken> AuthMiddleware::verifyToken(const std::string& token)
{
    try
    {
        auto decoded = jwt::decode(token);

        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256 { m_secretKey })
                            .with_issuer("farado-api");

        verifier.verify(decoded);

        JWTToken result;
        result.token = token;
        result.userId = decoded.get_payload_claim("user_id").as_string();
        result.expiresAt = std::chrono::system_clock::from_time_t(
            decoded.get_expires_at().time_since_epoch().count()
        );

        return result;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Token verification failed: " << e.what() << std::endl;
    }
    return std::nullopt;
}

void AuthMiddleware::invalidateToken(const std::string& token)
{
    auto jwtToken = verifyToken(token);
    if (!jwtToken.has_value())
    {
        std::cerr << "Cannot invalidate invalid token" << std::endl;
        return;
    }

    std::lock_guard<std::mutex> lock(m_blacklistMutex);
    m_blacklist[token] = jwtToken->expiresAt;

    // Очищаем черный список
    auto now = std::chrono::system_clock::now();
    for (auto it = m_blacklist.begin(); it != m_blacklist.end();)
    {
        if (it->second < now)
        {
            it = m_blacklist.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool AuthMiddleware::isTokenInvalidated(const std::string& token)
{
    std::lock_guard<std::mutex> lock(m_blacklistMutex);

    auto it = m_blacklist.find(token);
    if (it == m_blacklist.end())
    {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    if (it->second < now)
    {
        m_blacklist.erase(it);
        return false;
    }

    return true;
}

std::string AuthMiddleware::extractBearerToken(const std::string& authHeader)
{
    std::regex bearerRegex(R"(^Bearer\s+([a-zA-Z0-9\-_\.]+)$)");
    std::smatch matches;

    if (std::regex_match(authHeader, matches, bearerRegex) && matches.size() > 1)
    {
        return matches[1].str();
    }

    return "";
}

} // namespace server
} // namespace farado
