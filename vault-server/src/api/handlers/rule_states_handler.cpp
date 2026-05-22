#include <cpprest/uri.h>
#include <regex>

#include "common/dto/rule_state.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"
#include "rule_states_handler.h"

namespace server::handlers
{

RuleStatesHandler::RuleStatesHandler(std::shared_ptr<services::IRuleStateService> service)
    : m_service(std::move(service))
{
}

void RuleStatesHandler::handleGetItems(const web::http::http_request& request, const std::string& /*userId*/)
{
    auto params = extractQueryParams(request);
    int page = params.count("page") ? std::stoi(params["page"]) : 1;
    int pageSize = params.count("pageSize") ? std::stoi(params["pageSize"]) : 20;
    std::optional<int64_t> ruleId, stateId;
    if (params.count("ruleId"))
        ruleId = std::stoll(params["ruleId"]);
    if (params.count("stateId"))
        stateId = std::stoll(params["stateId"]);

    auto pageData = m_service->getRuleStates(page, pageSize, ruleId, stateId);
    web::json::value response;
    web::json::value items = web::json::value::array();
    for (size_t i = 0; i < pageData.items.size(); ++i)
        items[i] = dto::toWebJson(pageData.items[i].toJson());
    response["items"] = items;
    response["totalCount"] = pageData.totalCount;
    response["page"] = page;
    response["pageSize"] = pageSize;
    request.reply(web::http::status_codes::OK, response);
}

void RuleStatesHandler::handleGetItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractId(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    auto item = m_service->getRuleState(id);
    if (!item)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RuleState not found");
        request.reply(resp);
        return;
    }
    request.reply(web::http::status_codes::OK, dto::toWebJson(item->toJson()));
}

void RuleStatesHandler::handleCreateItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    request.extract_json().then([this, request](pplx::task<web::json::value> task)
                                {
        try {
            auto json = task.get();
            dto::RuleState rs(dto::toNlohmannJson(json));
            if (!rs.ruleId || !rs.stateId) {
                web::http::http_response resp(web::http::status_codes::BadRequest);
                sendErrorResponse(resp, 400, "ruleId and stateId are required");
                request.reply(resp);
                return;
            }
            auto created = m_service->createRuleState(rs);
            if (!created) {
                web::http::http_response resp(web::http::status_codes::Conflict);
                sendErrorResponse(resp, 409, "RuleState already exists or invalid references");
                request.reply(resp);
                return;
            }
            request.reply(web::http::status_codes::Created, dto::toWebJson(created->toJson()));
        } catch (const std::exception& e) {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
            request.reply(resp);
        } })
        .wait();
}

void RuleStatesHandler::handleUpdateItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractId(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    request.extract_json().then([this, request, id](pplx::task<web::json::value> task)
                                {
        try {
            auto json = task.get();
            auto nl = dto::toNlohmannJson(json);
            nl["id"] = id;
            dto::RuleState rs(nl);
            auto updated = m_service->updateRuleState(rs);
            if (!updated) {
                web::http::http_response resp(web::http::status_codes::NotFound);
                sendErrorResponse(resp, 404, "RuleState not found or update failed");
                request.reply(resp);
                return;
            }
            request.reply(web::http::status_codes::OK, dto::toWebJson(updated->toJson()));
        } catch (const std::exception& e) {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
            request.reply(resp);
        } })
        .wait();
}

void RuleStatesHandler::handleDeleteItem(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractId(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }
    if (m_service->deleteRuleState(id))
        request.reply(web::http::status_codes::NoContent);
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RuleState not found");
        request.reply(resp);
    }
}

int64_t RuleStatesHandler::extractId(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/rule-states/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
        return std::stoll(matches[1].str());
    return -1;
}

std::map<std::string, std::string> RuleStatesHandler::extractQueryParams(const web::http::http_request& request)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
        params[p.first] = p.second;
    return params;
}

void RuleStatesHandler::sendErrorResponse(web::http::http_response& response, int code, const std::string& message)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace server::handlers
