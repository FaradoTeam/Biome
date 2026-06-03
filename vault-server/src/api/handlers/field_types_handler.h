#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/ifield_type_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с типами полей.
 */
class FieldTypesHandler final : public BaseHandler
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
    std::shared_ptr<services::IFieldTypeService> m_fieldTypeService;
};

} // namespace handlers
} // namespace server
