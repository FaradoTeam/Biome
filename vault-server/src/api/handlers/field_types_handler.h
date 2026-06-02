#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/ifield_type_service.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с типами полей.
 */
class FieldTypesHandler final
{
public:
    explicit FieldTypesHandler(
        std::shared_ptr<services::IFieldTypeService> fieldTypeService
    );

    void handleGetFieldTypes(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleGetFieldType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleCreateFieldType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleUpdateFieldType(
        const web::http::http_request& request,
        const std::string& userId
    );

    void handleDeleteFieldType(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    int64_t extractFieldTypeIdFromPath(const web::http::http_request& request);
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
    std::shared_ptr<services::IFieldTypeService> m_fieldTypeService;
};

} // namespace handlers
} // namespace server
