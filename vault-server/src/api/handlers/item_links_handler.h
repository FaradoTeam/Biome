#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_link_service.h"

#include "base_handler.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы со связями элементов.
 */
class ItemLinksHandler final : public BaseHandler
{
public:
    explicit ItemLinksHandler(std::shared_ptr<services::IItemLinkService> itemLinkService);

    void handleGetItemLinks(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItemLink(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItemLinksByItemId(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItemLinksByLinkTypeId(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateItemLink(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteItemLink(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IItemLinkService> m_itemLinkService;
};

} // namespace server::handlers
