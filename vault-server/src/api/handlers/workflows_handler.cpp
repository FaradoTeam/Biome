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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }

    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    auto workflowsPage = m_workflowService->workflows(page, pageSize);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < workflowsPage.workflows.size(); ++i)
    {
        items[i] = dto::toWebJson(workflowsPage.workflows[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(workflowsPage.totalCount);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void WorkflowsHandler::handleGetWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid workflow ID");
        request.reply(resp);
        return;
    }

    auto workflow = m_workflowService->workflow(id);
    if (!workflow)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Workflow not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(workflow->toJson())
    );
}

void WorkflowsHandler::handleCreateWorkflow(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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

                    if (!workflow.caption || workflow.caption->empty())
                    {
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto workflowsPage = m_workflowService->workflows(1, 100);
                    bool exists = false;
                    for (const auto& wf : workflowsPage.workflows)
                    {
                        if (wf.caption == workflow.caption)
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (exists)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Conflict
                        );
                        sendErrorResponse(
                            resp, 409,
                            "Workflow with this caption already exists"
                        );
                        request.reply(resp);
                        return;
                    }

                    auto created = m_workflowService->createWorkflow(workflow, userId);
                    if (!created)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Forbidden
                        );
                        sendErrorResponse(
                            resp, 403,
                            "Insufficient permissions to create workflow"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(
                        web::http::status_codes::BadRequest
                    );
                    sendErrorResponse(
                        resp, 400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid workflow ID");
        request.reply(resp);
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
                        web::http::http_response resp(
                            web::http::status_codes::NotFound
                        );
                        sendErrorResponse(
                            resp, 404,
                            "Workflow not found or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(
                        web::http::status_codes::BadRequest
                    );
                    sendErrorResponse(
                        resp, 400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid workflow ID");
        request.reply(resp);
        return;
    }

    auto result = m_workflowService->deleteWorkflow(id, userId);
    if (!result.success)
    {
        web::http::http_response resp(
            static_cast<web::http::status_code>(result.errorCode)
        );
        sendErrorResponse(resp, result.errorCode, result.errorMessage);
        request.reply(resp);
        return;
    }

    request.reply(web::http::status_codes::NoContent);
}

} // namespace handlers
} // namespace server
