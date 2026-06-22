#include <regex>

#include <cpprest/uri.h>

#include "common/dto/link_type.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "link_types_handler.h"

namespace server::handlers
{

LinkTypesHandler::LinkTypesHandler(
    std::shared_ptr<services::ILinkTypeService> linkTypeService
)
    : m_linkTypeService(std::move(linkTypeService))
{
    if (!m_linkTypeService)
    {
        LOG_WARN << "LinkTypesHandler инициализирован без LinkTypeService";
    }
}

void LinkTypesHandler::handleGetLinkTypes(
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

    auto params = extractQueryParams(request);

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::optional<int64_t> sourceItemTypeId = std::nullopt;
    if (params.count("sourceItemTypeId"))
        sourceItemTypeId = std::stoll(params["sourceItemTypeId"]);

    std::optional<int64_t> destinationItemTypeId = std::nullopt;
    if (params.count("destinationItemTypeId"))
        destinationItemTypeId = std::stoll(params["destinationItemTypeId"]);

    LOG_DEBUG
        << "GET /link-types: user=" << *userIdOpt
        << ", page=" << page << ", pageSize=" << pageSize
        << ", sourceItemTypeId=" << (sourceItemTypeId.has_value() ? std::to_string(*sourceItemTypeId) : "none")
        << ", destinationItemTypeId=" << (destinationItemTypeId.has_value() ? std::to_string(*destinationItemTypeId) : "none");

    try
    {
        auto linkTypesPage = m_linkTypeService->getLinkTypes(
            page, pageSize, sourceItemTypeId, destinationItemTypeId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < linkTypesPage.linkTypes.size(); ++i)
        {
            items[i] = dto::toWebJson(linkTypesPage.linkTypes[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(linkTypesPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка типов связей: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void LinkTypesHandler::handleGetLinkType(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid link type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /link-types/" << id << " from user " << *userIdOpt;

    try
    {
        auto linkType = m_linkTypeService->getLinkType(id);
        if (!linkType)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Link type not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(linkType->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении типа связи " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void LinkTypesHandler::handleCreateLinkType(
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

    LOG_DEBUG << "POST /link-types from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::LinkType linkType(nlohmannJson);

                    if (!linkType.caption.has_value() || linkType.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!linkType.sourceItemTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "sourceItemTypeId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!linkType.destinationItemTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "destinationItemTypeId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_linkTypeService->createLinkType(linkType, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Insufficient permissions to create link type"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал тип связи id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании типа связи: " << e.what();
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

void LinkTypesHandler::handleUpdateLinkType(
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
        sendErrorResponse(resp, 400, "Invalid link type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /link-types/" << id << " from user " << userId;

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
                    dto::LinkType linkType(nlohmannJson);

                    auto updated = m_linkTypeService->updateLinkType(linkType, userId);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(
                            resp,
                            404,
                            "Link type not found or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил тип связи id=" << id;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении типа связи " << id << ": " << e.what();
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

void LinkTypesHandler::handleDeleteLinkType(
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
        sendErrorResponse(resp, 400, "Invalid link type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /link-types/" << id << " from user " << userId;

    try
    {
        if (m_linkTypeService->deleteLinkType(id, userId))
        {
            LOG_INFO << "Пользователь " << userId << " удалил тип связи id=" << id;
            request.reply(web::http::status_codes::NoContent);
        }
        else
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Link type not found or insufficient permissions");
            request.reply(resp);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении типа связи " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace server::handlers
