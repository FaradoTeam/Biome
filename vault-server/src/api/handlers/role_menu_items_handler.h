#pragma once

#include <map>
#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/irole_menu_item_service.h"

#include "base_handler.h"

namespace server::handlers
{

class RoleMenuItemsHandler final : public BaseHandler
{
public:
    explicit RoleMenuItemsHandler(std::shared_ptr<services::IRoleMenuItemService> service);

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
    std::shared_ptr<services::IRoleMenuItemService> m_service;
};

} // namespace server::handlers
