#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irule_service.h"

#include "base_handler.h"

namespace server::handlers
{

class RulesHandler final : public BaseHandler
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
    std::shared_ptr<services::IRuleService> m_ruleService;
};

} // namespace server::handlers
