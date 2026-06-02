#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_type_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с типами элементов.
 */
class ItemTypesHandler final
{
public:
    explicit ItemTypesHandler(
        std::shared_ptr<services::IItemTypeService> itemTypeService
    );

    void handleGetItemTypes(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetItemType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateItemType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateItemType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteItemType(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractItemTypeIdFromPath(const web::http::http_request& request);
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

private:
    std::shared_ptr<services::IItemTypeService> m_itemTypeService;
};

} // namespace handlers
} // namespace server
