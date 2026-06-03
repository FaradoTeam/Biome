#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iedge_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с переходами.
 */
class EdgesHandler final : public BaseHandler
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
    std::shared_ptr<services::IEdgeService> m_edgeService;
};

} // namespace handlers
} // namespace server
