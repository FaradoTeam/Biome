#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/ilink_type_service.h"

#include "base_handler.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы с типами связей.
 */
class LinkTypesHandler final : public BaseHandler
{
public:
    explicit LinkTypesHandler(std::shared_ptr<services::ILinkTypeService> linkTypeService);

    void handleGetLinkTypes(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetLinkType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateLinkType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateLinkType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteLinkType(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::ILinkTypeService> m_linkTypeService;
};

} // namespace server::handlers
