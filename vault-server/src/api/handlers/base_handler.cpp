#include <regex>

#include <cpprest/uri.h>

#include "base_handler.h"

namespace server
{
namespace handlers
{

int64_t BaseHandler::extractIdFromPath(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    // Ищем последнюю последовательность цифр в пути
    // Паттерн: /цифры в конце строки или перед слешем
    static const std::regex pattern(R"(/(\d+)(?:/|$))");
    std::smatch matches;

    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            return std::stoll(matches[1].str());
        }
        catch (const std::exception&)
        {
            return -1;
        }
    }
    return -1;
}

std::map<std::string, std::string> BaseHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
    {
        params[p.first] = p.second;
    }
    return params;
}

void BaseHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

std::optional<int64_t> BaseHandler::parseUserId(
    const std::string& userIdStr,
    web::http::http_response& response
)
{
    if (userIdStr.empty())
    {
        sendErrorResponse(response, 401, "User not authenticated");
        return std::nullopt;
    }

    try
    {
        int64_t userId = std::stoll(userIdStr);
        if (userId <= 0)
        {
            sendErrorResponse(response, 400, "Invalid user ID");
            return std::nullopt;
        }
        return userId;
    }
    catch (const std::exception&)
    {
        sendErrorResponse(response, 400, "Invalid user ID format");
        return std::nullopt;
    }
}

std::optional<bool> BaseHandler::parseBool(const std::string& value)
{
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on")
    {
        return true;
    }
    if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off")
    {
        return false;
    }
    return std::nullopt;
}

void BaseHandler::parsePaginationParams(
    const web::http::http_request& request,
    int& page,
    int& pageSize,
    int defaultPageSize,
    int maxPageSize
)
{
    auto params = extractQueryParams(request);

    // Парсим page
    page = 1;
    auto it = params.find("page");
    if (it != params.end())
    {
        try
        {
            int parsedPage = std::stoi(it->second);
            if (parsedPage > 0)
            {
                page = parsedPage;
            }
        }
        catch (const std::exception&)
        {
            // Оставляем значение по умолчанию
        }
    }

    // Парсим pageSize
    pageSize = defaultPageSize;
    it = params.find("pageSize");
    if (it != params.end())
    {
        try
        {
            int parsedPageSize = std::stoi(it->second);
            if (parsedPageSize > 0)
            {
                pageSize = std::min(parsedPageSize, maxPageSize);
            }
        }
        catch (const std::exception&)
        {
            // Оставляем значение по умолчанию
        }
    }
}

int64_t BaseHandler::parseIntParam(
    const std::map<std::string, std::string>& params,
    const std::string& key,
    int64_t defaultValue
)
{
    auto it = params.find(key);
    if (it == params.end())
    {
        return defaultValue;
    }

    try
    {
        return std::stoll(it->second);
    }
    catch (const std::exception&)
    {
        return defaultValue;
    }
}

std::string BaseHandler::parseStringParam(
    const std::map<std::string, std::string>& params,
    const std::string& key,
    const std::string& defaultValue
)
{
    auto it = params.find(key);
    if (it == params.end())
    {
        return defaultValue;
    }
    return it->second;
}

} // namespace handlers
} // namespace server
