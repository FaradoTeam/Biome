#include <regex>

#include <cpprest/uri.h>

#include "common/dto/item_user_state.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "item_user_states_handler.h"

namespace server
{
namespace handlers
{

ItemUserStatesHandler::ItemUserStatesHandler(
    std::shared_ptr<services::IItemUserStateService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "ItemUserStatesHandler инициализирован без сервиса";
    }
}

void ItemUserStatesHandler::handleGetItemUserStates(
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
            LOG_WARN << "handleGetItemUserStates: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetItemUserStates: неверный параметр pageSize: " << params["pageSize"];
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
            LOG_WARN << "handleGetItemUserStates: неверный параметр itemId: " << params["itemId"];
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
            LOG_WARN << "handleGetItemUserStates: неверный параметр userId: " << params["userId"];
        }
    }

    LOG_DEBUG
        << "GET /items/user-states: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", itemId=" << (itemId.has_value() ? std::to_string(*itemId) : "none")
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none");

    try
    {
        auto pageData = m_service->getItemUserStates(
            page, pageSize, userId, itemId, filterUserId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.states.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.states[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка ItemUserState: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemUserStatesHandler::handleGetItemUserState(
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
        sendErrorResponse(resp, 400, "Invalid state ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /items/user-states/" << id << " from user " << userId;

    try
    {
        auto state = m_service->getItemUserState(id, userId);
        if (!state)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Item user state not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(state->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ItemUserState " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemUserStatesHandler::handleGetLastItemUserState(
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

    // Извлекаем itemId из пути: /items/{itemId}/user-states/last
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/user-states/last)");
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid item ID");
            request.reply(resp);
            return;
        }
    }

    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/user-states/last from user " << userId;

    try
    {
        auto last = m_service->getLastItemUserState(itemId, userId);
        if (!last)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "No states found for this item");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(last->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении последнего ItemUserState для элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemUserStatesHandler::handleCreateItemUserState(
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

    // Извлекаем itemId из пути: /items/{itemId}/user-states
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/user-states)");
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid item ID");
            request.reply(resp);
            return;
        }
    }

    if (itemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /items/" << itemId << "/user-states from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, itemId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ItemUserState state(nlohmannJson);

                    // Убеждаемся, что itemId в пути и в теле совпадают
                    state.itemId = itemId;
                    state.userId = userId;

                    // Валидация
                    if (!state.stateId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "stateId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createItemUserState(state, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create item user state: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал ItemUserState для элемента " << itemId
                        << " с состоянием " << *created->stateId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ItemUserState: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

void ItemUserStatesHandler::handleDeleteItemUserState(
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
        sendErrorResponse(resp, 400, "Invalid state ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /items/user-states/" << id << " from user " << userId;

    try
    {
        auto result = m_service->deleteItemUserState(id, userId);
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
            << " удалил ItemUserState id=" << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ItemUserState " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
