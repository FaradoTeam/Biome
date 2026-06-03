#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irule_state_service.h"

#include "base_handler.h"

namespace server::handlers
{

class RuleStatesHandler final : public BaseHandler
{
public:
    explicit RuleStatesHandler(std::shared_ptr<services::IRuleStateService> service);

    void handleGetItems(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteItem(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IRuleStateService> m_service;
};

} // namespace server::handlers
