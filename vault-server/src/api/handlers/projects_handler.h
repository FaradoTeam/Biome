#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iproject_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с проектами (CRUD операции).
 */
class ProjectsHandler final
{
public:
    /**
     * @brief Конструктор обработчика.
     * @param projectService Указатель на сервис проектов.
     */
    explicit ProjectsHandler(
        std::shared_ptr<services::IProjectService> projectService
    );

    /**
     * @brief Обрабатывает запрос на получение списка проектов.
     */
    void handleGetProjects(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на получение конкретного проекта.
     */
    void handleGetProject(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на создание нового проекта.
     */
    void handleCreateProject(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на обновление существующего проекта.
     */
    void handleUpdateProject(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на архивацию/удаление проекта.
     */
    void handleDeleteProject(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    /**
     * @brief Извлекает ID проекта из пути запроса.
     * @param request HTTP-запрос
     * @return ID проекта или -1 при ошибке
     */
    int64_t extractProjectIdFromPath(const web::http::http_request& request);

    /**
     * @brief Извлекает userId из строки и конвертирует в число.
     * @param userIdStr Строковый ID пользователя
     * @param response HTTP-ответ для отправки ошибки
     * @return ID пользователя или std::nullopt при ошибке
     */
    std::optional<int64_t> parseUserId(
        const std::string& userIdStr,
        web::http::http_response& response
    );

    /**
     * @brief Извлекает query-параметры из запроса.
     */
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );

    /**
     * @brief Формирует и отправляет JSON-ответ с ошибкой.
     */
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

private:
    std::shared_ptr<services::IProjectService> m_projectService;
};

} // namespace handlers
} // namespace server
