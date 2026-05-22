#include <cpprest/uri.h>
#include <regex>

#include "common/dto/rule.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"
#include "rules_handler.h"

namespace server::handlers
{

RulesHandler::RulesHandler(std::shared_ptr<services::IRuleService> ruleService)
    : m_ruleService(std::move(ruleService))
{
    if (!m_ruleService)
        LOG_WARN << "RulesHandler без RuleService";
}

void RulesHandler::handleGetRules(const web::http::http_request& request, const std::string& /*userId*/)
{
    auto params = extractQueryParams(request);
    int page = params.count("page") ? std::stoi(params["page"]) : 1;
    int pageSize = params.count("pageSize") ? std::stoi(params["pageSize"]) : 20;
    std::optional<int64_t> roleId;
    if (params.count("roleId"))
        roleId = std::stoll(params["roleId"]);

    auto pageData = m_ruleService->getRules(page, pageSize, roleId);
    web::json::value response;
    web::json::value items = web::json::value::array();
    for (size_t i = 0; i < pageData.rules.size(); ++i)
        items[i] = dto::toWebJson(pageData.rules[i].toJson());
    response["items"] = items;
    response["totalCount"] = pageData.totalCount;
    response["page"] = page;
    response["pageSize"] = pageSize;
    request.reply(web::http::status_codes::OK, response);
}

void RulesHandler::handleGetRule(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractRuleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid rule ID");
        request.reply(resp);
        return;
    }
    auto rule = m_ruleService->getRule(id);
    if (!rule)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Rule not found");
        request.reply(resp);
        return;
    }
    request.reply(web::http::status_codes::OK, dto::toWebJson(rule->toJson()));
}

void RulesHandler::handleCreateRule(const web::http::http_request& request, const std::string& /*userId*/)
{
    request.extract_json().then([this, request](pplx::task<web::json::value> task)
                                {
        try {
            auto json = task.get();
            dto::Rule rule(dto::toNlohmannJson(json));
            if (!rule.roleId.has_value()) {
                web::http::http_response resp(web::http::status_codes::BadRequest);
                sendErrorResponse(resp, 400, "roleId is required");
                request.reply(resp);
                return;
            }
            auto created = m_ruleService->createRule(rule);
            if (!created) {
                web::http::http_response resp(web::http::status_codes::Conflict);
                sendErrorResponse(resp, 409, "Rule for this role already exists or invalid data");
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

void RulesHandler::handleUpdateRule(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractRuleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid rule ID");
        request.reply(resp);
        return;
    }
    request.extract_json().then([this, request, id](pplx::task<web::json::value> task)
                                {
        try {
            auto json = task.get();
            auto nlohmannJson = dto::toNlohmannJson(json);
            nlohmannJson["id"] = id;
            dto::Rule rule(nlohmannJson);
            auto updated = m_ruleService->updateRule(rule);
            if (!updated) {
                web::http::http_response resp(web::http::status_codes::NotFound);
                sendErrorResponse(resp, 404, "Rule not found or update failed");
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

void RulesHandler::handleDeleteRule(const web::http::http_request& request, const std::string& /*userId*/)
{
    int64_t id = extractRuleIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid rule ID");
        request.reply(resp);
        return;
    }
    if (m_ruleService->deleteRule(id))
        request.reply(web::http::status_codes::NoContent);
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Rule not found");
        request.reply(resp);
    }
}

int64_t RulesHandler::extractRuleIdFromPath(const web::http::http_request& request)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/rules/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
        return std::stoll(matches[1].str());
    return -1;
}

std::map<std::string, std::string> RulesHandler::extractQueryParams(const web::http::http_request& request)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
        params[p.first] = p.second;
    return params;
}

void RulesHandler::sendErrorResponse(web::http::http_response& response, int code, const std::string& message)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

} // namespace server::handlers
