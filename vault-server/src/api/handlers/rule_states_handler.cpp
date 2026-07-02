#include <cpprest/uri.h>

#include "common/dto/rule_state.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "rule_states_handler.h"

namespace server::handlers
{

RuleStatesHandler::RuleStatesHandler(
    std::shared_ptr<services::IRuleStateService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "RuleStatesHandler инициализирован без сервиса";
    }
}

void RuleStatesHandler::handleGetItems(
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

    std::optional<int64_t> ruleId;
    if (params.count("ruleId"))
        ruleId = std::stoll(params["ruleId"]);

    std::optional<int64_t> stateId;
    if (params.count("stateId"))
        stateId = std::stoll(params["stateId"]);

    try
    {
        auto pageData = m_service->getRuleStates(page, pageSize, ruleId, stateId);

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.items.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.items[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка RuleState: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RuleStatesHandler::handleGetItem(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid ID");
        return;
    }

    try
    {
        auto item = m_service->getRuleState(id);
        if (!item)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "RuleState not found");
            return;
        }

        sendJsonResponse(
            request,
            web::http::status_codes::OK,
            dto::toWebJson(item->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении RuleState " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RuleStatesHandler::handleCreateItem(
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
                    auto json = task.get();
                    dto::RuleState rs(dto::toNlohmannJson(json));

                    if (!rs.ruleId.has_value() || !rs.stateId.has_value())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "ruleId and stateId are required"
                        );
                        return;
                    }

                    auto created = m_service->createRuleState(rs, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create RuleState"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал RuleState id=" << *created->id
                        << ", ruleId=" << *created->ruleId
                        << ", stateId=" << *created->stateId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании RuleState: " << e.what();
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

void RuleStatesHandler::handleUpdateItem(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid ID");
        return;
    }

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::RuleState rs(nl);

                    auto updated = m_service->updateRuleState(rs, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "RuleState not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил RuleState id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении RuleState: " << e.what();
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

void RuleStatesHandler::handleDeleteItem(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid ID");
        return;
    }

    LOG_DEBUG << "DELETE /rule-states/" << id << " from user " << userId;

    try
    {
        if (m_service->deleteRuleState(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил RuleState id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "RuleState not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении RuleState " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
