#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <optional>
#include <mutex>

namespace httplib
{
class Request;
}

namespace farado
{
namespace server
{

struct JWTToken
{
    std::string token;
    std::string userId;
    std::chrono::system_clock::time_point expiresAt;
};

class AuthMiddleware
{
public:
    using Ptr = std::shared_ptr<AuthMiddleware>;

    explicit AuthMiddleware(const std::string& secretKey);
    ~AuthMiddleware() = default;

    bool validateRequest(const httplib::Request& req, std::string& userId);
    std::string generateToken(const std::string& userId, int expiresInSeconds = 3600);
    void invalidateToken(const std::string& token);
    bool isTokenInvalidated(const std::string& token);

private:
    std::string extractBearerToken(const std::string& authHeader);
    std::optional<JWTToken> verifyToken(const std::string& token);

private:
    std::string m_secretKey;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> m_blacklist;
    std::mutex m_blacklistMutex;
};

} // namespace server
} // namespace farado
