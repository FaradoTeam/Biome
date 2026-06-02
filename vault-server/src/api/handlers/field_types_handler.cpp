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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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

    response["items"] = items;
    response["totalCount"] = web::json::value::number(fieldTypesPage.totalCount);
    response["page"] = web::json::value::number(page);
    response["pageSize"] = web::json::value::number(pageSize);

    request.reply(web::http::status_codes::OK, response);
}

void FieldTypesHandler::handleGetFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }

    int64_t id = extractFieldTypeIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid field type ID");
        request.reply(resp);
        return;
    }

    auto fieldType = m_fieldTypeService->fieldType(id);
    if (!fieldType)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Field type not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(fieldType->toJson())
    );
}

void FieldTypesHandler::handleCreateFieldType(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    int64_t userId = *userIdOpt;

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
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!fieldType.itemTypeId.has_value())
                    {
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(resp, 400, "itemTypeId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!fieldType.valueType.has_value() || fieldType.valueType->empty())
                    {
                        web::http::http_response resp(
                            web::http::status_codes::BadRequest
                        );
                        sendErrorResponse(resp, 400, "valueType is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_fieldTypeService->createFieldType(fieldType, userId);
                    if (!created)
                    {
                        web::http::http_response resp(
                            web::http::status_codes::Forbidden
                        );
                        sendErrorResponse(
                            resp,
                            403,
                            "Insufficient permissions to create field type"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    int64_t userId = *userIdOpt;

    const int64_t id = extractFieldTypeIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid field type ID");
        request.reply(resp);
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
                        web::http::http_response resp(
                            web::http::status_codes::NotFound
                        );
                        sendErrorResponse(
                            resp,
                            404,
                            "Field type not found or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    web::http::http_response resp(
                        web::http::status_codes::BadRequest
                    );
                    sendErrorResponse(
                        resp,
                        400,
                        std::string("Invalid request: ") + e.what()
                    );
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    int64_t userId = *userIdOpt;

    const int64_t id = extractFieldTypeIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid field type ID");
        request.reply(resp);
        return;
    }

    if (m_fieldTypeService->deleteFieldType(id, userId))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "Field type not found or insufficient permissions");
        request.reply(resp);
    }
}

int64_t FieldTypesHandler::extractFieldTypeIdFromPath(
    const web::http::http_request& request
)
{
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/field-types/(\d+))");
    std::smatch matches;
    if (std::regex_match(path, matches, pattern) && matches.size() > 1)
    {
        return std::stoll(matches[1].str());
    }
    return -1;
}

std::map<std::string, std::string> FieldTypesHandler::extractQueryParams(
    const web::http::http_request& request
)
{
    std::map<std::string, std::string> params;
    auto query = web::uri::split_query(request.request_uri().query());
    for (const auto& p : query)
    {
        params[p.first] = p.second;
    }
    return params;
}

void FieldTypesHandler::sendErrorResponse(
    web::http::http_response& response,
    int code,
    const std::string& message
)
{
    web::json::value error;
    error["code"] = web::json::value::number(code);
    error["message"] = web::json::value::string(message);
    response.set_body(error);
}

std::optional<int64_t> FieldTypesHandler::parseUserId(
    const std::string& userIdStr,
    web::http::http_response& response
)
{
    if (userIdStr.empty())
    {
        sendErrorResponse(response, 401, "User not authenticated");
        return std::nullopt;
    }

    try
    {
        int64_t userId = std::stoll(userIdStr);
        if (userId <= 0)
        {
            sendErrorResponse(response, 400, "Invalid user ID");
            return std::nullopt;
        }
        return userId;
    }
    catch (const std::exception&)
    {
        sendErrorResponse(response, 400, "Invalid user ID format");
        return std::nullopt;
    }
}

} // namespace handlers
} // namespace server
