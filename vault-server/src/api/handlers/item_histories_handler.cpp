#include <regex>

#include <cpprest/uri.h>

#include "common/dto/item_history.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "item_histories_handler.h"

namespace server
{
namespace handlers
{

ItemHistoriesHandler::ItemHistoriesHandler(
    std::shared_ptr<services::IItemHistoryService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "ItemHistoriesHandler инициализирован без сервиса";
    }
}

void ItemHistoriesHandler::handleGetItemHistories(
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
            LOG_WARN << "handleGetItemHistories: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetItemHistories: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> itemId = std::nullopt;
    if (params.count("itemId"))
    {
        try
        {
            itemId = std::stoll(params["itemId"]);
            if (itemId <= 0)
                itemId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetItemHistories: неверный параметр itemId: " << params["itemId"];
        }
    }

    std::optional<int64_t> filterUserId = std::nullopt;
    if (params.count("userId"))
    {
        try
        {
            filterUserId = std::stoll(params["userId"]);
            if (filterUserId <= 0)
                filterUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetItemHistories: неверный параметр userId: " << params["userId"];
        }
    }

    // Фильтры по дате
    std::optional<common::DateTime> dateFrom = std::nullopt;
    if (params.count("dateFrom"))
    {
        try
        {
            int64_t timestamp = std::stoll(params["dateFrom"]);
            dateFrom = common::secondsToTimePoint(timestamp);
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetItemHistories: неверный параметр dateFrom: " << params["dateFrom"];
        }
    }

    std::optional<common::DateTime> dateTo = std::nullopt;
    if (params.count("dateTo"))
    {
        try
        {
            int64_t timestamp = std::stoll(params["dateTo"]);
            dateTo = common::secondsToTimePoint(timestamp);
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetItemHistories: неверный параметр dateTo: " << params["dateTo"];
        }
    }

    LOG_DEBUG
        << "GET /items/histories: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", itemId=" << (itemId.has_value() ? std::to_string(*itemId) : "none")
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none");

    try
    {
        auto pageData = m_service->getItemHistories(
            page, pageSize, userId, itemId, filterUserId, dateFrom, dateTo
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.histories.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.histories[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка ItemHistory: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemHistoriesHandler::handleGetItemHistory(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid history ID");
        return;
    }

    LOG_DEBUG << "GET /items/histories/" << id << " from user " << userId;

    try
    {
        auto history = m_service->getItemHistory(id, userId);
        if (!history)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Item history not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(history->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ItemHistory " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemHistoriesHandler::handleGetLastItemHistory(
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

    // Извлекаем itemId из пути: /items/{itemId}/histories/last
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/histories/last)");
    std::smatch matches;

    int64_t itemId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/histories/last from user " << userId;

    try
    {
        auto last = m_service->getLastItemHistory(itemId, userId);
        if (!last)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "No history found for this item");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(last->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении последнего ItemHistory для элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemHistoriesHandler::handleCreateItemHistory(
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

    // Извлекаем itemId из пути: /items/{itemId}/histories
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/histories)");
    std::smatch matches;

    int64_t itemId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "POST /items/" << itemId << "/histories from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, itemId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ItemHistory history(nlohmannJson);

                    // Убеждаемся, что itemId в пути и в теле совпадают
                    history.itemId = itemId;
                    history.userId = userId;

                    auto created = m_service->createItemHistory(history, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create item history: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал ItemHistory для элемента " << itemId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ItemHistory: " << e.what();
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

void ItemHistoriesHandler::handleDeleteItemHistory(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid history ID");
        return;
    }

    LOG_DEBUG << "DELETE /items/histories/" << id << " from user " << userId;

    try
    {
        auto result = m_service->deleteItemHistory(id, userId);
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
            << " удалил ItemHistory id=" << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ItemHistory " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
