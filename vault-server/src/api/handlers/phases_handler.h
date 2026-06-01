#pragma once

#include <memory>
#include <map>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iphase_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с фазами (CRUD операции).
 */
class PhasesHandler final
{
public:
    /**
     * @brief Конструктор обработчика.
     * @param phaseService Указатель на сервис фаз (содержит логику проверки прав)
     */
    explicit PhasesHandler(std::shared_ptr<services::IPhaseService> phaseService);

    /**
     * @brief Обрабатывает запрос на получение списка фаз.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleGetPhases(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на получение конкретной фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleGetPhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на создание новой фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleCreatePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на обновление существующей фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleUpdatePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на архивацию (удаление) фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleDeletePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    /**
     * @brief Извлекает ID фазы из пути запроса.
     * @param request HTTP-запрос
     * @return ID фазы или -1 при ошибке
     */
    int64_t extractPhaseIdFromPath(const web::http::http_request& request);

    /**
     * @brief Извлекает query-параметры из запроса.
     * @param request HTTP-запрос
     * @return Словарь с параметрами и их значениями
     */
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );

    /**
     * @brief Парсит userId из строки.
     * @param userIdStr Строковое представление ID пользователя
     * @param response HTTP-ответ для отправки ошибки
     * @return ID пользователя или std::nullopt при ошибке
     */
    std::optional<int64_t> parseUserId(
        const std::string& userIdStr,
        web::http::http_response& response
    );

    /**
     * @brief Отправляет ошибку в формате JSON.
     * @param response HTTP-ответ для модификации
     * @param code Код ошибки
     * @param message Текст сообщения об ошибке
     */
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

    /**
     * @brief Парсит булево значение из строки.
     * @param value Строковое значение ("true"/"false"/"1"/"0")
     * @return std::optional<bool> или std::nullopt если значение не распознано
     */
    std::optional<bool> parseBool(const std::string& value);

private:
    std::shared_ptr<services::IPhaseService> m_phaseService;
};

} // namespace handlers
} // namespace server
