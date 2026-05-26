#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iteam_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с командами.
 */
class TeamsHandler final
{
public:
    explicit TeamsHandler(std::shared_ptr<services::ITeamService> teamService);

    void handleGetTeams(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetTeam(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateTeam(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateTeam(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteTeam(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractTeamIdFromPath(const web::http::http_request& request);
    std::map<std::string, std::string> extractQueryParams(
        const web::http::http_request& request
    );
    void sendErrorResponse(
        web::http::http_response& response,
        int code,
        const std::string& message
    );

private:
    std::shared_ptr<services::ITeamService> m_teamService;
};

} // namespace handlers
} // namespace server
