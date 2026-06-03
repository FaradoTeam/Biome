#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_type_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с типами элементов.
 */
class ItemTypesHandler final : public BaseHandler
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
    std::shared_ptr<services::IItemTypeService> m_itemTypeService;
};

} // namespace handlers
} // namespace server
