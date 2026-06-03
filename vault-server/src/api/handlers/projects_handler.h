#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iproject_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с проектами (CRUD операции).
 */
class ProjectsHandler final : public BaseHandler
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
    std::shared_ptr<services::IProjectService> m_projectService;
};

} // namespace handlers
} // namespace server
