#include <cpprest/uri.h>

#include "common/dto/item.h"
#include "common/dto/item_field.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "items_handler.h"

namespace server
{
namespace handlers
{

ItemsHandler::ItemsHandler(std::shared_ptr<services::IItemService> itemService)
    : m_itemService(std::move(itemService))
{
    if (!m_itemService)
    {
        LOG_WARN << "ItemsHandler инициализирован без ItemService";
    }
}

// ============================================================
// GET /api/items
// ============================================================

void ItemsHandler::handleGetItems(
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
            LOG_WARN
                << "handleGetItems: неверный параметр page: " << params["page"];
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
            LOG_WARN
                << "handleGetItems: неверный параметр pageSize: "
                << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> itemTypeId = std::nullopt;
    if (params.count("itemTypeId"))
    {
        try
        {
            itemTypeId = std::stoll(params["itemTypeId"]);
            if (itemTypeId <= 0)
                itemTypeId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetItems: неверный параметр itemTypeId: "
                << params["itemTypeId"];
        }
    }

    std::optional<int64_t> parentId = std::nullopt;
    if (params.count("parentId"))
    {
        try
        {
            parentId = std::stoll(params["parentId"]);
            if (parentId <= 0)
                parentId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetItems: неверный параметр parentId: "
                << params["parentId"];
        }
    }

    std::optional<int64_t> phaseId = std::nullopt;
    if (params.count("phaseId"))
    {
        try
        {
            phaseId = std::stoll(params["phaseId"]);
            if (phaseId <= 0)
                phaseId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetItems: неверный параметр phaseId: "
                << params["phaseId"];
        }
    }

    std::optional<int64_t> stateId = std::nullopt;
    if (params.count("stateId"))
    {
        try
        {
            stateId = std::stoll(params["stateId"]);
            if (stateId <= 0)
                stateId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN
                << "handleGetItems: неверный параметр stateId: "
                << params["stateId"];
        }
    }

    std::optional<bool> isDeleted = std::nullopt;
    if (params.count("isDeleted"))
    {
        isDeleted = parseBool(params["isDeleted"]);
    }

    std::string searchCaption;
    if (params.count("searchCaption"))
    {
        searchCaption = params["searchCaption"];
    }

    LOG_DEBUG
        << "GET /api/items: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", itemTypeId=" << (itemTypeId.has_value() ? std::to_string(*itemTypeId) : "none")
        << ", parentId=" << (parentId.has_value() ? std::to_string(*parentId) : "none")
        << ", phaseId=" << (phaseId.has_value() ? std::to_string(*phaseId) : "none")
        << ", stateId=" << (stateId.has_value() ? std::to_string(*stateId) : "none")
        << ", isDeleted=" << (isDeleted.has_value() ? (*isDeleted ? "true" : "false") : "none")
        << ", searchCaption=" << searchCaption;

    try
    {
        auto itemsPage = m_itemService->items(
            page, pageSize, userId,
            itemTypeId, parentId, phaseId, stateId, isDeleted, searchCaption
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < itemsPage.items.size(); ++i)
        {
            items[i] = dto::toWebJson(itemsPage.items[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(itemsPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка элементов: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /api/items/{id}
// ============================================================

void ItemsHandler::handleGetItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /api/items/" << itemId << " from user " << userId;

    try
    {
        auto item = m_itemService->item(itemId, userId);
        if (!item)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Item not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(item->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /api/items
// ============================================================

void ItemsHandler::handleCreateItem(
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

    LOG_DEBUG << "POST /api/items from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Item item(nlohmannJson);

                    // Валидация обязательных полей
                    if (!item.caption.has_value() || item.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Item caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!item.itemTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "itemTypeId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!item.stateId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "stateId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!item.phaseId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "phaseId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_itemService->createItem(item, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create item: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал элемент id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании элемента: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// PUT /api/items/{id}
// ============================================================

void ItemsHandler::handleUpdateItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /api/items/" << itemId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, itemId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = itemId;
                    dto::Item item(nlohmannJson);

                    auto updated = m_itemService->updateItem(item, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или элемент не найден
                        auto existing = m_itemService->item(itemId, userId);
                        if (!existing)
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Item not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this item");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил элемент " << itemId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении элемента " << itemId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /api/items/{id}
// ============================================================

void ItemsHandler::handleDeleteItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /api/items/" << itemId << " from user " << userId;

    try
    {
        auto result = m_itemService->deleteItem(itemId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил элемент " << itemId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /api/items/{id}/restore
// ============================================================

void ItemsHandler::handleRestoreItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /api/items/" << itemId << "/restore from user " << userId;

    try
    {
        auto result = m_itemService->restoreItem(itemId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " восстановил элемент " << itemId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при восстановлении элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /api/items/{id}/fields
// ============================================================

void ItemsHandler::handleGetItemFields(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /api/items/" << itemId << "/fields from user " << userId;

    try
    {
        auto fields = m_itemService->getItemFields(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < fields.size(); ++i)
        {
            response[i] = dto::toWebJson(fields[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении полей элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// PUT /api/items/{id}/fields/{fieldTypeId}
// ============================================================

void ItemsHandler::handleSetItemField(
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

    // Извлекаем itemId и fieldTypeId из пути
    // Ожидаем формат: /api/items/{itemId}/fields/{fieldTypeId}
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/items/(\d+)/fields/(\d+))");
    std::smatch matches;

    int64_t itemId = -1;
    int64_t fieldTypeId = -1;

    if (std::regex_match(path, matches, pattern) && matches.size() >= 3)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
            fieldTypeId = std::stoll(matches[2].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid item ID or field type ID");
            request.reply(resp);
            return;
        }
    }

    if (itemId <= 0 || fieldTypeId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID or field type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG
        << "PUT /api/items/" << itemId << "/fields/" << fieldTypeId
        << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, itemId, fieldTypeId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    dto::ItemField field;
                    field.itemId = itemId;
                    field.fieldTypeId = fieldTypeId;

                    if (nlohmannJson.contains("value"))
                    {
                        field.value = nlohmannJson["value"].get<std::string>();
                    }

                    auto created = m_itemService->setItemField(field, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot set field value: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " установил поле " << fieldTypeId
                        << " для элемента " << itemId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при установке поля элемента: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /api/items/{id}/fields/{fieldTypeId}
// ============================================================

void ItemsHandler::handleDeleteItemField(
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

    // Извлекаем itemId и fieldTypeId из пути
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/api/items/(\d+)/fields/(\d+))");
    std::smatch matches;

    int64_t itemId = -1;
    int64_t fieldTypeId = -1;

    if (std::regex_match(path, matches, pattern) && matches.size() >= 3)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
            fieldTypeId = std::stoll(matches[2].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid item ID or field type ID");
            request.reply(resp);
            return;
        }
    }

    if (itemId <= 0 || fieldTypeId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID or field type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG
        << "DELETE /api/items/" << itemId << "/fields/" << fieldTypeId
        << " from user " << userId;

    try
    {
        auto result = m_itemService->deleteItemField(itemId, fieldTypeId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил поле " << fieldTypeId
            << " для элемента " << itemId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении поля элемента: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
