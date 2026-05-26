#include <regex>

#include <cpprest/uri.h>

#include "common/dto/team.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "teams_handler.h"

namespace server
{
namespace handlers
{

TeamsHandler::TeamsHandler(std::shared_ptr<services::ITeamService> teamService)
    : m_teamService(std::move(teamService))
{
    if (!m_teamService)
    {
        LOG_WARN << "TeamsHandler инициализирован без TeamService";
    }
}

void TeamsHandler::handleGetTeams(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::string searchCaption;
    if (params.count("searchCaption"))
        searchCaption = params["searchCaption"];

    auto teamsPage = m_teamService->getTeams(page, pageSize, searchCaption);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < teamsPage.teams.size(); ++i)
    {
        items[i] = dto::toWebJson(teamsPage.teams[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = web::json::value::number(teamsPage.totalCount);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void TeamsHandler::handleGetTeam(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    int64_t id = extractTeamIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid team ID");
        request.reply(resp);
        return;
    }

    auto team = m_teamService->getTeam(id);
    if (!team)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Team not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(team->toJson())
    );
}

void TeamsHandler::handleCreateTeam(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    request
        .extract_json()
        .then(
            [this, request](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Team team(nlohmannJson);

                    if (!team.caption || team.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_teamService->createTeam(team);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Could not create team");
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
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void TeamsHandler::handleUpdateTeam(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractTeamIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid team ID");
        request.reply(resp);
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::Team team(nlohmannJson);

                    auto updated = m_teamService->updateTeam(team);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "Team not found or update failed");
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
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void TeamsHandler::handleDeleteTeam(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractTeamIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid team ID");
        request.reply(resp);
        return;
    }

    if (m_teamService->deleteTeam(id))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Team not found");
        request.reply(resp);
    }
}

int64_t TeamsHandler::extractTeamIdFromPath(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/teams/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
    {
        return std::stoll(matches[1].str());
    }
    return -1;
}

std::map<std::string, std::string> TeamsHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
    {
        params[p.first] = p.second;
    }
    return params;
}

void TeamsHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace handlers
} // namespace server
