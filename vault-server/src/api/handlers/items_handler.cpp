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
// GET /items
// ============================================================

void ItemsHandler::handleGetItems(
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
        << "GET /items: user=" << userId
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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(itemsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка элементов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /items/{id}
// ============================================================

void ItemsHandler::handleGetItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << " from user " << userId;

    try
    {
        auto item = m_itemService->item(itemId, userId);
        if (!item)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Item not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(item->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /items
// ============================================================

void ItemsHandler::handleCreateItem(
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

    LOG_DEBUG << "POST /items from user " << userId;

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
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Item caption is required");
                        return;
                    }

                    if (!item.itemTypeId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "itemTypeId is required");
                        return;
                    }

                    if (!item.stateId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "stateId is required");
                        return;
                    }

                    if (!item.phaseId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "phaseId is required");
                        return;
                    }

                    auto created = m_itemService->createItem(item, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create item: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал элемент id=" << *created->id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании элемента: " << e.what();
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

// ============================================================
// PUT /items/{id}
// ============================================================

void ItemsHandler::handleUpdateItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "PUT /items/" << itemId << " from user " << userId;

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
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Item not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this item"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил элемент " << itemId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении элемента " << itemId << ": " << e.what();
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

// ============================================================
// DELETE /items/{id}
// ============================================================

void ItemsHandler::handleDeleteItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "DELETE /items/" << itemId << " from user " << userId;

    try
    {
        auto result = m_itemService->deleteItem(itemId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил элемент " << itemId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /items/{id}/restore
// ============================================================

void ItemsHandler::handleRestoreItem(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "POST /items/" << itemId << "/restore from user " << userId;

    try
    {
        auto result = m_itemService->restoreItem(itemId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " восстановил элемент " << itemId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при восстановлении элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /items/{id}/fields
// ============================================================

void ItemsHandler::handleGetItemFields(
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

    const int64_t itemId = extractIdFromPath(request);
    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/fields from user " << userId;

    try
    {
        auto fields = m_itemService->getItemFields(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < fields.size(); ++i)
        {
            response[i] = dto::toWebJson(fields[i].toJson());
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении полей элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// PUT /items/{id}/fields/{fieldTypeId}
// ============================================================

void ItemsHandler::handleSetItemField(
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

    // Извлекаем itemId и fieldTypeId из пути
    // Ожидаем формат: /items/{itemId}/fields/{fieldTypeId}
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/fields/(\d+))");
    std::smatch matches;

    int64_t itemId = -1;
    int64_t fieldTypeId = -1;

    if (std::regex_search(path, matches, pattern) && matches.size() >= 3)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
            fieldTypeId = std::stoll(matches[2].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID or field type ID");
            return;
        }
    }

    if (itemId <= 0 || fieldTypeId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID or field type ID");
        return;
    }

    LOG_DEBUG
        << "PUT /items/" << itemId << "/fields/" << fieldTypeId
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
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot set field value: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " установил поле " << fieldTypeId
                        << " для элемента " << itemId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при установке поля элемента: " << e.what();
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

// ============================================================
// DELETE /items/{id}/fields/{fieldTypeId}
// ============================================================

void ItemsHandler::handleDeleteItemField(
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

    // Извлекаем itemId и fieldTypeId из пути
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/fields/(\d+))");
    std::smatch matches;

    int64_t itemId = -1;
    int64_t fieldTypeId = -1;

    if (std::regex_search(path, matches, pattern) && matches.size() >= 3)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
            fieldTypeId = std::stoll(matches[2].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID or field type ID");
            return;
        }
    }

    if (itemId <= 0 || fieldTypeId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID or field type ID");
        return;
    }

    LOG_DEBUG
        << "DELETE /items/" << itemId << "/fields/" << fieldTypeId
        << " from user " << userId;

    try
    {
        auto result = m_itemService->deleteItemField(itemId, fieldTypeId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил поле " << fieldTypeId
            << " для элемента " << itemId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении поля элемента: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
