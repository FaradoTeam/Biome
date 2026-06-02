#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irule_service.h"

namespace server::handlers
{

class RulesHandler final
{
public:
    explicit RulesHandler(std::shared_ptr<services::IRuleService> ruleService);

    void handleGetRules(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetRule(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateRule(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateRule(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteRule(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractRuleIdFromPath(const web::http::http_request& request);
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

    std::shared_ptr<services::IRuleService> m_ruleService;
};

} // namespace server::handlers
