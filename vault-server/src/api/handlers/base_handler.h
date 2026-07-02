#pragma once

#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <regex>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

namespace server
{
namespace handlers
{

/**
 * @brief Базовый класс для всех обработчиков API.
 *
 * Предоставляет общие утилитарные методы для работы с HTTP-запросами.
 */
class BaseHandler
{
public:
    virtual ~BaseHandler() = default;

protected:
    /**
     * @brief Извлекает числовой ID из пути запроса.
     */
    int64_t extractIdFromPath(const web::http::http_request& request);

    /**
     * @brief Извлекает все query-параметры из запроса.
     */
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );

    /**
     * @brief Парсит строку с ID пользователя в int64_t.
     * @param userIdStr Строковое представление ID пользователя
     * @return ID пользователя или std::nullopt при ошибке
     */
    std::optional<int64_t> parseUserId(const std::string& userIdStr);

    /**
     * @brief Парсит булево значение из строки.
     */
    std::optional<bool> parseBool(const std::string& value);

    /**
     * @brief Парсит параметры пагинации из запроса.
     */
    void parsePaginationParams(
        const web::http::http_request& request,
        int& page,
        int& pageSize,
        int defaultPageSize = 20,
        int maxPageSize = 100
    );

    /**
     * @brief Парсит целочисленный параметр из словаря.
     */
    int64_t parseIntParam(
        const std::map<std::string, std::string>& params,
        const std::string& key,
        int64_t defaultValue = 0
    );

    /**
     * @brief Парсит строковый параметр из словаря.
     */
    std::string parseStringParam(
        const std::map<std::string, std::string>& params,
        const std::string& key,
        const std::string& defaultValue = ""
    );

    /**
     * @brief Отправляет HTTP-ответ с добавлением CORS-заголовков.
     */
    void sendResponse(
        const web::http::http_request& request,
        web::http::http_response& response
    );

    /**
     * @brief Отправляет JSON-ответ с указанным статусом.
     */
    void sendJsonResponse(
        const web::http::http_request& request,
        web::http::status_code status,
        const web::json::value& body
    );

    /**
     * @brief Отправляет ответ с кодом ошибки и текстовым сообщением.
     */
    void sendErrorResponse(
        const web::http::http_request& request,
        web::http::status_code status,
        const std::string& message
    );
};

} // namespace handlers
} // namespace server
