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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(linkTypesPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка типов связей: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void LinkTypesHandler::handleGetLinkType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid link type ID");
        return;
    }

    LOG_DEBUG << "GET /link-types/" << id << " from user " << *userIdOpt;

    try
    {
        auto linkType = m_linkTypeService->getLinkType(id);
        if (!linkType)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Link type not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(linkType->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении типа связи " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void LinkTypesHandler::handleCreateLinkType(
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
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Caption is required");
                        return;
                    }

                    if (!linkType.sourceItemTypeId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "sourceItemTypeId is required");
                        return;
                    }

                    if (!linkType.destinationItemTypeId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "destinationItemTypeId is required");
                        return;
                    }

                    auto created = m_linkTypeService->createLinkType(linkType, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to create link type"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал тип связи id=" << *created->id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании типа связи: " << e.what();
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

void LinkTypesHandler::handleUpdateLinkType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid link type ID");
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
                        sendErrorResponse(
                            request,
                            web::http::status_codes::NotFound,
                            "Link type not found or insufficient permissions"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил тип связи id=" << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении типа связи " << id << ": " << e.what();
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

void LinkTypesHandler::handleDeleteLinkType(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid link type ID");
        return;
    }

    LOG_DEBUG << "DELETE /link-types/" << id << " from user " << userId;

    try
    {
        if (m_linkTypeService->deleteLinkType(id, userId))
        {
            LOG_INFO << "Пользователь " << userId << " удалил тип связи id=" << id;

            web::http::http_response response(web::http::status_codes::NoContent);
            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(
                request,
                web::http::status_codes::NotFound,
                "Link type not found or insufficient permissions"
            );
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении типа связи " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
