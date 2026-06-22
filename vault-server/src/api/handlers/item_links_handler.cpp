#include <regex>

#include <cpprest/uri.h>

#include "common/dto/item_link.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "item_links_handler.h"

namespace server::handlers
{

ItemLinksHandler::ItemLinksHandler(
    std::shared_ptr<services::IItemLinkService> itemLinkService
)
    : m_itemLinkService(std::move(itemLinkService))
{
    if (!m_itemLinkService)
    {
        LOG_WARN << "ItemLinksHandler инициализирован без ItemLinkService";
    }
}

void ItemLinksHandler::handleGetItemLinks(
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

    int page = 1;
    if (params.count("page"))
        page = std::stoi(params["page"]);

    int pageSize = 20;
    if (params.count("pageSize"))
        pageSize = std::stoi(params["pageSize"]);

    std::optional<int64_t> linkTypeId = std::nullopt;
    if (params.count("linkTypeId"))
        linkTypeId = std::stoll(params["linkTypeId"]);

    std::optional<int64_t> sourceItemId = std::nullopt;
    if (params.count("sourceItemId"))
        sourceItemId = std::stoll(params["sourceItemId"]);

    std::optional<int64_t> destinationItemId = std::nullopt;
    if (params.count("destinationItemId"))
        destinationItemId = std::stoll(params["destinationItemId"]);

    LOG_DEBUG
        << "GET /item-links: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", linkTypeId=" << (linkTypeId.has_value() ? std::to_string(*linkTypeId) : "none")
        << ", sourceItemId=" << (sourceItemId.has_value() ? std::to_string(*sourceItemId) : "none")
        << ", destinationItemId=" << (destinationItemId.has_value() ? std::to_string(*destinationItemId) : "none");

    try
    {
        auto linksPage = m_itemLinkService->getItemLinks(
            page, pageSize, userId, linkTypeId, sourceItemId, destinationItemId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < linksPage.links.size(); ++i)
        {
            items[i] = dto::toWebJson(linksPage.links[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(linksPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка связей элементов: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemLinksHandler::handleGetItemLink(
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
        sendErrorResponse(resp, 400, "Invalid item link ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /item-links/" << id << " from user " << userId;

    try
    {
        auto link = m_itemLinkService->getItemLink(id, userId);
        if (!link)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Item link not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(link->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связи элемента " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemLinksHandler::handleGetItemLinksByItemId(
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

    // Извлекаем itemId из пути: /items/{itemId}/links
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/links)");
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

    LOG_DEBUG << "GET /items/" << itemId << "/links from user " << userId;

    try
    {
        auto links = m_itemLinkService->getItemLinksByItemId(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связей элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemLinksHandler::handleGetItemLinksByLinkTypeId(
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

    // Извлекаем linkTypeId из пути: /link-types/{linkTypeId}/links
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/link-types/(\d+)/links)");
    std::smatch matches;

    int64_t linkTypeId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            linkTypeId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid link type ID");
            request.reply(resp);
            return;
        }
    }

    if (linkTypeId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid link type ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /link-types/" << linkTypeId << "/links from user " << userId;

    try
    {
        auto links = m_itemLinkService->getItemLinksByLinkTypeId(linkTypeId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связей для типа " << linkTypeId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

void ItemLinksHandler::handleCreateItemLink(
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

    LOG_DEBUG << "POST /item-links from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ItemLink itemLink(nlohmannJson);

                    if (!itemLink.linkTypeId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "linkTypeId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!itemLink.sourceItemId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "sourceItemId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!itemLink.destinationItemId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "destinationItemId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_itemLinkService->createItemLink(itemLink, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create item link: insufficient permissions or duplicate link"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал связь элементов id=" << *created->id
                        << " (type=" << *created->linkTypeId
                        << ", source=" << *created->sourceItemId
                        << ", dest=" << *created->destinationItemId << ")";

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании связи элементов: " << e.what();
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

void ItemLinksHandler::handleDeleteItemLink(
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
        sendErrorResponse(resp, 400, "Invalid item link ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /item-links/" << id << " from user " << userId;

    try
    {
        auto result = m_itemLinkService->deleteItemLink(id, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил связь элементов id=" << id;
        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении связи элементов " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace server::handlers
