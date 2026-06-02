#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iedge_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с переходами.
 */
class EdgesHandler final
{
public:
    explicit EdgesHandler(std::shared_ptr<services::IEdgeService> edgeService);

    void handleGetEdges(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetEdge(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateEdge(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteEdge(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все переходы для рабочего процесса.
     */
    void handleGetWorkflowEdges(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractEdgeIdFromPath(const web::http::http_request& request);
    int64_t extractWorkflowIdFromPath(const web::http::http_request& request);
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );
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
    std::shared_ptr<services::IEdgeService> m_edgeService;
};

} // namespace handlers
} // namespace server
