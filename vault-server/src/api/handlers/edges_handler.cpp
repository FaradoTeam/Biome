#include <regex>

#include <cpprest/uri.h>

#include "common/dto/edge.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "edges_handler.h"

namespace server
{
namespace handlers
{

EdgesHandler::EdgesHandler(
    std::shared_ptr<services::IEdgeService> edgeService
)
    : m_edgeService(std::move(edgeService))
{
    if (!m_edgeService)
    {
        LOG_WARN << "EdgesHandler инициализирован без EdgeService";
    }
}

void EdgesHandler::handleGetEdges(
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

    std::optional<int64_t> beginStateId;
    if (params.count("beginStateId"))
        beginStateId = std::stoll(params["beginStateId"]);

    std::optional<int64_t> endStateId;
    if (params.count("endStateId"))
        endStateId = std::stoll(params["endStateId"]);

    auto edgesPage = m_edgeService->edges(page, pageSize, beginStateId, endStateId);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < edgesPage.edges.size(); ++i)
    {
        items[i] = dto::toWebJson(edgesPage.edges[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(edgesPage.totalCount);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void EdgesHandler::handleGetEdge(
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
        sendErrorResponse(resp, 400, "Invalid edge ID");
        request.reply(resp);
        return;
    }

    auto edge = m_edgeService->edge(id);
    if (!edge)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Edge not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(edge->toJson())
    );
}

void EdgesHandler::handleCreateEdge(
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
                    dto::Edge edge(dto::toNlohmannJson(jsonBody));

                    if (!edge.beginStateId || !edge.endStateId)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(
                            resp, 400,
                            "beginStateId and endStateId are required"
                        );
                        request.reply(resp);
                        return;
                    }

                    auto edgesPage = m_edgeService->edges(1, 1, edge.beginStateId, edge.endStateId);
                    if (!edgesPage.edges.empty())
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Conflict
                        );
                        sendErrorResponse(
                            resp, 409,
                            "Edge already exists"
                        );
                        request.reply(resp);
                        return;
                    }

                    auto created = m_edgeService->createEdge(edge, userId);
                    if (!created)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Forbidden
                        );
                        sendErrorResponse(
                            resp, 403,
                            "Insufficient permissions to create edge"
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

void EdgesHandler::handleDeleteEdge(
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
        sendErrorResponse(resp, 400, "Invalid edge ID");
        request.reply(resp);
        return;
    }

    auto result = m_edgeService->deleteEdge(id, userId);
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

void EdgesHandler::handleGetWorkflowEdges(
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

    const int64_t workflowId = extractIdFromPath(request);
    if (workflowId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid workflow ID");
        request.reply(resp);
        return;
    }

    auto edges = m_edgeService->getWorkflowEdges(workflowId);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < edges.size(); ++i)
    {
        items[i] = dto::toWebJson(edges[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(edges.size());

    request.reply(web::http::status_codes::OK, response);
}

} // namespace handlers
} // namespace server
