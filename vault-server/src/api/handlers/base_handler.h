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
 * Предоставляет общие утилитарные методы для работы с HTTP-запросами:
 * - Извлечение ID из пути
 * - Парсинг query-параметров
 * - Формирование JSON-ответов с ошибками
 * - Парсинг ID пользователя из строки
 * - Парсинг булевых значений
 * - Парсинг параметров пагинации
 */
class BaseHandler
{
public:
    virtual ~BaseHandler() = default;

protected:
    /**
     * @brief Извлекает числовой ID из пути запроса.
     *
     * Ищет последнюю последовательность цифр в пути.
     * Примеры: /api/users/123 -> 123, /api/projects/456/edit -> 456
     *
     * @param request HTTP-запрос
     * @return ID или -1 при ошибке
     */
    int64_t extractIdFromPath(const web::http::http_request& request);

    /**
     * @brief Извлекает все query-параметры из запроса.
     *
     * @param request HTTP-запрос
     * @return Словарь с параметрами и их значениями
     */
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );

    /**
     * @brief Формирует и отправляет JSON-ответ с ошибкой.
     *
     * @param response HTTP-ответ для модификации
     * @param code HTTP-код ошибки
     * @param message Текст сообщения об ошибке
     */
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

    /**
     * @brief Парсит строку с ID пользователя в int64_t.
     *
     * @param userIdStr Строковое представление ID пользователя
     * @param response HTTP-ответ для отправки ошибки
     * @return ID пользователя или std::nullopt при ошибке
     */
    std::optional<int64_t> parseUserId(
        const std::string& userIdStr,
        web::http::http_response& response
    );

    /**
     * @brief Парсит булево значение из строки.
     *
     * Поддерживает форматы: "true"/"false", "1"/"0", "yes"/"no", "on"/"off"
     *
     * @param value Строковое значение
     * @return std::optional<bool> или std::nullopt если значение не распознано
     */
    std::optional<bool> parseBool(const std::string& value);

    /**
     * @brief Парсит параметры пагинации из запроса.
     *
     * @param request HTTP-запрос
     * @param page Сюда будет записан номер страницы
     * @param pageSize Сюда будет записан размер страницы
     * @param defaultPageSize Значение pageSize по умолчанию (20)
     * @param maxPageSize Максимальное значение pageSize (100)
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
     *
     * @param params Словарь с параметрами
     * @param key Ключ параметра
     * @param defaultValue Значение по умолчанию
     * @return Значение параметра или defaultValue при ошибке
     */
    int64_t parseIntParam(
        const std::map<std::string, std::string>& params,
        const std::string& key,
        int64_t defaultValue = 0
    );

    /**
     * @brief Парсит строковый параметр из словаря.
     *
     * @param params Словарь с параметрами
     * @param key Ключ параметра
     * @param defaultValue Значение по умолчанию
     * @return Значение параметра или defaultValue если параметр отсутствует
     */
    std::string parseStringParam(
        const std::map<std::string, std::string>& params,
        const std::string& key,
        const std::string& defaultValue = ""
    );
};

} // namespace handlers
} // namespace server
