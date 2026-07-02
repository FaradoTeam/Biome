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

    std::optional<int64_t> beginStateId;
    if (params.count("beginStateId"))
        beginStateId = std::stoll(params["beginStateId"]);

    std::optional<int64_t> endStateId;
    if (params.count("endStateId"))
        endStateId = std::stoll(params["endStateId"]);

    try
    {
        auto edgesPage = m_edgeService->edges(page, pageSize, beginStateId, endStateId);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < edgesPage.edges.size(); ++i)
        {
            items[i] = dto::toWebJson(edgesPage.edges[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(edgesPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка переходов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void EdgesHandler::handleGetEdge(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid edge ID");
        return;
    }

    try
    {
        auto edge = m_edgeService->edge(id);
        if (!edge)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Edge not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(edge->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении перехода " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void EdgesHandler::handleCreateEdge(
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
                    dto::Edge edge(dto::toNlohmannJson(jsonBody));

                    if (!edge.beginStateId || !edge.endStateId)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "beginStateId and endStateId are required"
                        );
                        return;
                    }

                    auto edgesPage = m_edgeService->edges(1, 1, edge.beginStateId, edge.endStateId);
                    if (!edgesPage.edges.empty())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Conflict,
                            "Edge already exists"
                        );
                        return;
                    }

                    auto created = m_edgeService->createEdge(edge, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create edge"
                        );
                        return;
                    }

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании перехода: " << e.what();
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

void EdgesHandler::handleDeleteEdge(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid edge ID");
        return;
    }

    try
    {
        auto result = m_edgeService->deleteEdge(id, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO << "Переход удален: id=" << id << ", пользователь=" << userId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении перехода " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void EdgesHandler::handleGetWorkflowEdges(
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

    const int64_t workflowId = extractIdFromPath(request);
    if (workflowId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid workflow ID");
        return;
    }

    try
    {
        auto edges = m_edgeService->getWorkflowEdges(workflowId);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < edges.size(); ++i)
        {
            items[i] = dto::toWebJson(edges[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(edges.size());

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении переходов для рабочего процесса: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
