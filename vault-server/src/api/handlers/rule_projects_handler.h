#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irule_project_service.h"

#include "base_handler.h"

namespace server::handlers
{

class RuleProjectsHandler final : public BaseHandler
{
public:
    explicit RuleProjectsHandler(std::shared_ptr<services::IRuleProjectService> service);

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
    std::shared_ptr<services::IRuleProjectService> m_service;
};

} // namespace server::handlers
