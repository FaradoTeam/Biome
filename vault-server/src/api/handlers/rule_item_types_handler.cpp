#include <cpprest/uri.h>

#include "common/dto/rule_item_type.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "rule_item_types_handler.h"

namespace server::handlers
{

RuleItemTypesHandler::RuleItemTypesHandler(
    std::shared_ptr<services::IRuleItemTypeService> service
)
    : m_service(std::move(service))
{
    if (!m_service)
    {
        LOG_WARN << "RuleItemTypesHandler инициализирован без сервиса";
    }
}

void RuleItemTypesHandler::handleGetItems(
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

    std::optional<int64_t> itemTypeId;
    if (params.count("itemTypeId"))
        itemTypeId = std::stoll(params["itemTypeId"]);

    try
    {
        auto pageData = m_service->getRuleItemTypes(page, pageSize, ruleId, itemTypeId);

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
        LOG_ERROR << "Ошибка при получении списка RuleItemType: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RuleItemTypesHandler::handleGetItem(
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
        auto item = m_service->getRuleItemType(id);
        if (!item)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "RuleItemType not found");
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
        LOG_ERROR << "Ошибка при получении RuleItemType " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void RuleItemTypesHandler::handleCreateItem(
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
                    dto::RuleItemType rit(dto::toNlohmannJson(json));

                    if (!rit.ruleId.has_value() || !rit.itemTypeId.has_value())
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::BadRequest,
                            "ruleId and itemTypeId are required"
                        );
                        return;
                    }

                    auto created = m_service->createRuleItemType(rit, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create RuleItemType"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал RuleItemType id=" << *created->id
                        << ", ruleId=" << *created->ruleId
                        << ", itemTypeId=" << *created->itemTypeId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании RuleItemType: " << e.what();
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

void RuleItemTypesHandler::handleUpdateItem(
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
                    dto::RuleItemType rit(nl);

                    auto updated = m_service->updateRuleItemType(rit, userId);
                    if (!updated)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "RuleItemType not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил RuleItemType id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении RuleItemType: " << e.what();
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

void RuleItemTypesHandler::handleDeleteItem(
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

    LOG_DEBUG << "DELETE /rule-item-types/" << id << " from user " << userId;

    try
    {
        if (m_service->deleteRuleItemType(id, userId))
        {
            LOG_INFO
                << "Пользователь " << userId
                << " удалил RuleItemType id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "RuleItemType not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении RuleItemType " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
