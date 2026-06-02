#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/istate_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с состояниями.
 */
class StatesHandler final
{
public:
    explicit StatesHandler(
        std::shared_ptr<services::IStateService> stateService
    );

    void handleGetStates(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateState(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteState(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractStateIdFromPath(const web::http::http_request& request);
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
    std::shared_ptr<services::IStateService> m_stateService;
};

} // namespace handlers
} // namespace server
