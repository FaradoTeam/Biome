#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iworkflow_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с рабочими процессами.
 */
class WorkflowsHandler final
{
public:
    explicit WorkflowsHandler(
        std::shared_ptr<services::IWorkflowService> workflowService
    );

    /**
     * @brief Получает список рабочих процессов с пагинацией.
     */
    void handleGetWorkflows(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает рабочий процесс по ID.
     */
    void handleGetWorkflow(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создает новый рабочий процесс.
     */
    void handleCreateWorkflow(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующий рабочий процесс.
     */
    void handleUpdateWorkflow(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет рабочий процесс.
     */
    void handleDeleteWorkflow(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    /**
     * @brief Извлекает ID рабочего процесса из пути запроса.
     */
    int64_t extractWorkflowIdFromPath(const web::http::http_request& request);

    /**
     * @brief Извлекает query-параметры из запроса.
     */
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );

    /**
     * @brief Отправляет ошибку в формате JSON.
     */
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );
    std::optional<int64_t> parseUserId(
        const std::string& userIdStr,
        web::http::http_response& response
    );

private:
    std::shared_ptr<services::IWorkflowService> m_workflowService;
};

} // namespace handlers
} // namespace server
