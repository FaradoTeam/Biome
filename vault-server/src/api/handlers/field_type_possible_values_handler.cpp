#include <cpprest/uri.h>

#include "common/dto/field_type_possible_value.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "field_type_possible_values_handler.h"

namespace server
{
namespace handlers
{

FieldTypePossibleValuesHandler::FieldTypePossibleValuesHandler(
    std::shared_ptr<services::IFieldTypePossibleValueService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "FieldTypePossibleValuesHandler инициализирован без сервиса";
    }
}

void FieldTypePossibleValuesHandler::handleGetValues(
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
    const int64_t userId = *userIdOpt;

    auto params = extractQueryParams(request);

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetValues: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetValues: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтр по типу поля
    std::optional<int64_t> fieldTypeId = std::nullopt;
    if (params.count("fieldTypeId"))
    {
        try
        {
            fieldTypeId = std::stoll(params["fieldTypeId"]);
            if (fieldTypeId <= 0)
                fieldTypeId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetValues: неверный параметр fieldTypeId: " << params["fieldTypeId"];
        }
    }

    LOG_DEBUG
        << "GET /field-type-values: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", fieldTypeId=" << (fieldTypeId.has_value() ? std::to_string(*fieldTypeId) : "none");

    try
    {
        auto pageData = m_service->getFieldTypePossibleValues(
            page,
            pageSize,
            userId,
            fieldTypeId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.values.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.values[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка возможных значений: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void FieldTypePossibleValuesHandler::handleGetValue(
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
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid value ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /field-type-values/" << id << " from user " << userId;

    try
    {
        auto value = m_service->getFieldTypePossibleValue(id, userId);
        if (!value)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Field type possible value not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(value->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении возможного значения " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void FieldTypePossibleValuesHandler::handleCreateValue(
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
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /field-type-values from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::FieldTypePossibleValue value(nlohmannJson);

                    // Валидация обязательных полей
                    if (!value.fieldTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "fieldTypeId is required");
                        request.reply(resp);
                        return;
                    }
                    if (!value.value.has_value() || value.value->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "value is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createFieldTypePossibleValue(value, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Insufficient permissions to create field type possible value"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал возможное значение для поля typeId="
                        << *created->fieldTypeId << ", value='" << *created->value << "'";

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании возможного значения: " << e.what();
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

void FieldTypePossibleValuesHandler::handleUpdateValue(
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
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid value ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /field-type-values/" << id << " from user " << userId;

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
                    dto::FieldTypePossibleValue value(nlohmannJson);

                    // Валидация: value не может быть пустым
                    if (value.value.has_value() && value.value->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "value cannot be empty");
                        request.reply(resp);
                        return;
                    }

                    auto updated = m_service->updateFieldTypePossibleValue(value, userId);
                    if (!updated)
                    {
                        // Проверяем, существует ли запись
                        auto existing = m_service->getFieldTypePossibleValue(id, userId);
                        if (!existing)
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Field type possible value not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Insufficient permissions to update this value"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил возможное значение id=" << id;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR
                        << "Ошибка при обновлении возможного значения " << id
                        << ": " << e.what();
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

void FieldTypePossibleValuesHandler::handleDeleteValue(
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
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid value ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /field-type-values/" << id << " from user " << userId;

    try
    {
        auto result = m_service->deleteFieldTypePossibleValue(id, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил возможное значение id=" << id;
        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении возможного значения " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void FieldTypePossibleValuesHandler::handleGetValuesByFieldType(
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
    const int64_t userId = *userIdOpt;

    const int64_t fieldTypeId = extractIdFromPath(request);
    if (fieldTypeId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid field type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG
        << "GET /field-type-values/by-field-type/" << fieldTypeId
        << " from user " << userId;

    try
    {
        auto values = m_service->getValuesByFieldTypeId(fieldTypeId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < values.size(); ++i)
        {
            response[i] = dto::toWebJson(values[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка при получении значений для типа поля "
            << fieldTypeId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
