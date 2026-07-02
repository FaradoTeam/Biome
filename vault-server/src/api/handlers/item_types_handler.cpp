#include <cpprest/uri.h>

#include "common/dto/item_type.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "item_types_handler.h"

namespace server
{
namespace handlers
{

ItemTypesHandler::ItemTypesHandler(
    std::shared_ptr<services::IItemTypeService> itemTypeService
)
    : m_itemTypeService(std::move(itemTypeService))
{
    if (!m_itemTypeService)
    {
        LOG_WARN << "ItemTypesHandler инициализирован без ItemTypeService";
    }
}

void ItemTypesHandler::handleGetItemTypes(
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

    std::optional<int64_t> workflowId = std::nullopt;
    if (params.count("workflowId"))
        workflowId = std::stoll(params["workflowId"]);

    std::optional<std::string> kind = std::nullopt;
    if (params.count("kind"))
        kind = params["kind"];

    std::string searchCaption = "";
    if (params.count("searchCaption"))
        searchCaption = params["searchCaption"];

    try
    {
        auto itemTypesPage = m_itemTypeService->itemTypes(
            page, pageSize, workflowId, kind, searchCaption
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < itemTypesPage.itemTypes.size(); ++i)
        {
            items[i] = dto::toWebJson(itemTypesPage.itemTypes[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(itemTypesPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка типов элементов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemTypesHandler::handleGetItemType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item type ID");
        return;
    }

    try
    {
        auto itemType = m_itemTypeService->itemType(id);
        if (!itemType)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Item type not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(itemType->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении типа элемента " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemTypesHandler::handleCreateItemType(
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
                    dto::ItemType itemType(nlohmannJson);

                    if (!itemType.caption.has_value() || itemType.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Caption is required");
                        return;
                    }

                    if (!itemType.workflowId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "workflowId is required");
                        return;
                    }

                    if (!itemType.kind.has_value() || itemType.kind->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "kind is required");
                        return;
                    }

                    auto created = m_itemTypeService->createItemType(itemType, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create item type"
                        );
                        return;
                    }

                    LOG_INFO << "Создан новый тип элемента с id=" << *created->id << ", пользователь=" << userId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании типа элемента: " << e.what();
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

void ItemTypesHandler::handleUpdateItemType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item type ID");
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
                    dto::ItemType itemType(nlohmannJson);

                    auto updated = m_itemTypeService->updateItemType(itemType, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Item type not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO << "Тип элемента с id=" << id << " обновлен, пользователь=" << userId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении типа элемента " << id << ": " << e.what();
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

void ItemTypesHandler::handleDeleteItemType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item type ID");
        return;
    }

    try
    {
        if (m_itemTypeService->deleteItemType(id, userId))
        {
            LOG_INFO << "Тип элемента с id=" << id << " удален, пользователь=" << userId;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "Item type not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении типа элемента " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
