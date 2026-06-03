#include <regex>

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
    const std::string& /*userId*/
)
{
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

    auto pageData = m_service->getRuleItemTypes(page, pageSize, ruleId, itemTypeId);

    web::json::value response;
    web::json::value items = web::json::value::array();

    for (size_t i = 0; i < pageData.items.size(); ++i)
    {
        items[i] = dto::toWebJson(pageData.items[i].toJson());
    }

    response["items"] = items;
    response["totalCount"] = pageData.totalCount;
    response["page"] = page;
    response["pageSize"] = pageSize;

    request.reply(web::http::status_codes::OK, response);
}

void RuleItemTypesHandler::handleGetItem(
    const web::http::http_request& request,
    const std::string& /*userId*/
)
{
    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }

    auto item = m_service->getRuleItemType(id);
    if (!item)
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RuleItemType not found");
        request.reply(resp);
        return;
    }

    request.reply(
        web::http::status_codes::OK,
        dto::toWebJson(item->toJson())
    );
}

void RuleItemTypesHandler::handleCreateItem(
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
                    auto json = task.get();
                    dto::RuleItemType rit(dto::toNlohmannJson(json));

                    if (!rit.ruleId.has_value() || !rit.itemTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "ruleId and itemTypeId are required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_service->createRuleItemType(rit, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to create RuleItemType");
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

void RuleItemTypesHandler::handleUpdateItem(
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
        sendErrorResponse(resp, 400, "Invalid ID");
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
                    auto json = task.get();
                    auto nl = dto::toNlohmannJson(json);
                    nl["id"] = id;
                    dto::RuleItemType rit(nl);

                    auto updated = m_service->updateRuleItemType(rit, userId);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(resp, 404, "RuleItemType not found or insufficient permissions");
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

void RuleItemTypesHandler::handleDeleteItem(
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
        sendErrorResponse(resp, 400, "Invalid ID");
        request.reply(resp);
        return;
    }

    if (m_service->deleteRuleItemType(id, userId))
    {
        request.reply(web::http::status_codes::NoContent);
    }
    else
    {
        web::http::http_response resp(web::http::status_codes::NotFound);
        sendErrorResponse(resp, 404, "RuleItemType not found or insufficient permissions");
        request.reply(resp);
    }
}

} // namespace server::handlers
