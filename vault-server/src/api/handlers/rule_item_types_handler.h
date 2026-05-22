#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irule_item_type_service.h"

namespace server
{
namespace handlers
{

class RuleItemTypesHandler final
{
public:
    explicit RuleItemTypesHandler(std::shared_ptr<services::IRuleItemTypeService> service);
    void handleGetItems(const web::http::http_request& request, const std::string& userId);
    void handleGetItem(const web::http::http_request& request, const std::string& userId);
    void handleCreateItem(const web::http::http_request& request, const std::string& userId);
    void handleUpdateItem(const web::http::http_request& request, const std::string& userId);
    void handleDeleteItem(const web::http::http_request& request, const std::string& userId);

private:
    int64_t extractId(const web::http::http_request& request);
    std::map<std::string, std::string> extractQueryParams(const web::http::http_request& request);
    void sendErrorResponse(web::http::http_response& response, int code, const std::string& message);
    std::shared_ptr<services::IRuleItemTypeService> m_service;
};

} // namespace handlers
} // namespace server
