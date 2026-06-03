#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iworkflow_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с рабочими процессами.
 */
class WorkflowsHandler final : public BaseHandler
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
    std::shared_ptr<services::IWorkflowService> m_workflowService;
};

} // namespace handlers
} // namespace server
