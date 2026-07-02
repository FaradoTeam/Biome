#include <regex>

#include <cpprest/uri.h>

#include "common/dto/field_type.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "field_types_handler.h"

namespace server
{
namespace handlers
{

FieldTypesHandler::FieldTypesHandler(
    std::shared_ptr<services::IFieldTypeService> fieldTypeService
)
    : m_fieldTypeService(std::move(fieldTypeService))
{
    if (!m_fieldTypeService)
    {
        LOG_WARN << "FieldTypesHandler инициализирован без FieldTypeService";
    }
}

void FieldTypesHandler::handleGetFieldTypes(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }

    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::optional<int64_t> itemTypeId = std::nullopt;
    if (params.count("itemTypeId"))
        itemTypeId = std::stoll(params["itemTypeId"]);

    std::optional<std::string> valueType = std::nullopt;
    if (params.count("valueType"))
        valueType = params["valueType"];

    std::string searchCaption = "";
    if (params.count("searchCaption"))
        searchCaption = params["searchCaption"];

    try
    {
        auto fieldTypesPage = m_fieldTypeService->fieldTypes(
            page,
            pageSize,
            itemTypeId,
            valueType,
            searchCaption
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < fieldTypesPage.fieldTypes.size(); ++i)
        {
            items[i] = dto::toWebJson(fieldTypesPage.fieldTypes[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(fieldTypesPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка типов полей: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void FieldTypesHandler::handleGetFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid field type ID");
        return;
    }

    try
    {
        auto fieldType = m_fieldTypeService->fieldType(id);
        if (!fieldType)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Field type not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(fieldType->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении типа поля " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void FieldTypesHandler::handleCreateFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::FieldType fieldType(nlohmannJson);

                    if (!fieldType.caption.has_value() || fieldType.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Caption is required");
                        return;
                    }

                    if (!fieldType.itemTypeId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "itemTypeId is required");
                        return;
                    }

                    if (!fieldType.valueType.has_value() || fieldType.valueType->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "valueType is required");
                        return;
                    }

                    auto created = m_fieldTypeService->createFieldType(fieldType, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create field type"
                        );
                        return;
                    }

                    LOG_INFO << "Создан новый тип поля с id=" << *created->id << ", пользователь=" << userId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании типа поля: " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

void FieldTypesHandler::handleUpdateFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid field type ID");
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = id;
                    dto::FieldType fieldType(nlohmannJson);

                    auto updated = m_fieldTypeService->updateFieldType(fieldType, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Field type not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO << "Тип поля с id=" << id << " обновлен, пользователь=" << userId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении типа поля " << id << ": " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

void FieldTypesHandler::handleDeleteFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid field type ID");
        return;
    }

    try
    {
        if (m_fieldTypeService->deleteFieldType(id, userId))
        {
            LOG_INFO << "Тип поля с id=" << id << " удален, пользователь=" << userId;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "Field type not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении типа поля " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
