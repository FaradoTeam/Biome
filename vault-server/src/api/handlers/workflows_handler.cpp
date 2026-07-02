#include <cpprest/uri.h>

#include "common/dto/workflow.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "workflows_handler.h"

namespace server
{
namespace handlers
{

WorkflowsHandler::WorkflowsHandler(
    std::shared_ptr<services::IWorkflowService> workflowService
)
    : m_workflowService(std::move(workflowService))
{
    if (!m_workflowService)
    {
        LOG_WARN << "WorkflowsHandler инициализирован без WorkflowService";
    }
}

void WorkflowsHandler::handleGetWorkflows(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }

    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    try
    {
        auto workflowsPage = m_workflowService->workflows(page, pageSize);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < workflowsPage.workflows.size(); ++i)
        {
            items[i] = dto::toWebJson(workflowsPage.workflows[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(workflowsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка рабочих процессов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void WorkflowsHandler::handleGetWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid workflow ID");
        return;
    }

    try
    {
        auto workflow = m_workflowService->workflow(id);
        if (!workflow)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Workflow not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(workflow->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении рабочего процесса " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void WorkflowsHandler::handleCreateWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    dto::Workflow workflow(dto::toNlohmannJson(jsonBody));

                    if (!workflow.caption.has_value() || workflow.caption->empty())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "Caption is required"
                        );
                        return;
                    }

                    auto workflowsPage = m_workflowService->workflows(1, 100);
                    bool exists = false;
                    for (const auto& wf : workflowsPage.workflows)
                    {
                        if (wf.caption.has_value() && *wf.caption == *workflow.caption)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (exists)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Conflict,
                            "Workflow with this caption already exists"
                        );
                        return;
                    }

                    auto created = m_workflowService->createWorkflow(workflow, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create workflow"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал рабочий процесс id=" << *created->id
                        << ", caption=" << *created->caption;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании рабочего процесса: " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

void WorkflowsHandler::handleUpdateWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid workflow ID");
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    nlohmann::json nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::Workflow workflow(nlohmannJson);

                    auto updated = m_workflowService->updateWorkflow(workflow, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Workflow not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил рабочий процесс id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении рабочего процесса " << id << ": " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

void WorkflowsHandler::handleDeleteWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid workflow ID");
        return;
    }

    LOG_DEBUG << "DELETE /workflows/" << id << " from user " << userId;

    try
    {
        auto result = m_workflowService->deleteWorkflow(id, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил рабочий процесс id=" << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении рабочего процесса " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server