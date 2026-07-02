#include <cpprest/uri.h>

#include "common/dto/user_action.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_actions_handler.h"

namespace server
{
namespace handlers
{

UserActionsHandler::UserActionsHandler(
    std::shared_ptr<services::IUserActionService> actionService
)
    : m_actionService(std::move(actionService))
{
    if (!m_actionService)
    {
        LOG_WARN << "UserActionsHandler инициализирован без UserActionService";
    }
}

// ============================================================
// GET /user-actions
// ============================================================

void UserActionsHandler::handleGetActions(
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
            LOG_WARN << "handleGetActions: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetActions: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
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
            LOG_WARN << "handleGetActions: неверный параметр userId: " << params["userId"];
        }
    }

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
            LOG_WARN << "handleGetActions: неверный параметр dateFrom: " << params["dateFrom"];
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
            LOG_WARN << "handleGetActions: неверный параметр dateTo: " << params["dateTo"];
        }
    }

    LOG_DEBUG
        << "GET /user-actions: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none");

    try
    {
        auto actionsPage = m_actionService->getActions(
            page, pageSize, userId, filterUserId, dateFrom, dateTo
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < actionsPage.actions.size(); ++i)
        {
            items[i] = dto::toWebJson(actionsPage.actions[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(actionsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка действий: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /user-actions/{id}
// ============================================================

void UserActionsHandler::handleGetAction(
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

    const int64_t actionId = extractIdFromPath(request);
    if (actionId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid action ID");
        return;
    }

    LOG_DEBUG << "GET /user-actions/" << actionId << " from user " << userId;

    try
    {
        auto action = m_actionService->getAction(actionId, userId);
        if (!action.has_value())
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Action not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(action->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении действия " << actionId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /user-actions
// ============================================================

void UserActionsHandler::handleCreateAction(
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

    LOG_DEBUG << "POST /user-actions from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::UserAction action(nlohmannJson);

                    // Устанавливаем пользователя, если не указан
                    if (!action.userId.has_value())
                    {
                        action.userId = userId;
                    }

                    // Валидация обязательных полей
                    if (!action.caption.has_value() || action.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Action caption is required");
                        return;
                    }

                    auto created = m_actionService->createAction(action, userId);
                    if (!created.has_value())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create action: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал действие id=" << *created->id
                        << ", caption=" << *created->caption;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании действия: " << e.what();
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
// DELETE /user-actions/{id}
// ============================================================

void UserActionsHandler::handleDeleteAction(
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

    const int64_t actionId = extractIdFromPath(request);
    if (actionId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid action ID");
        return;
    }

    LOG_DEBUG << "DELETE /user-actions/" << actionId << " from user " << userId;

    try
    {
        auto result = m_actionService->deleteAction(actionId, userId);
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
            << " удалил действие " << actionId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении действия " << actionId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
