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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(linksPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка связей элементов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemLinksHandler::handleGetItemLink(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item link ID");
        return;
    }

    LOG_DEBUG << "GET /item-links/" << id << " from user " << userId;

    try
    {
        auto link = m_itemLinkService->getItemLink(id, userId);
        if (!link)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Item link not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(link->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связи элемента " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemLinksHandler::handleGetItemLinksByItemId(
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связей элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemLinksHandler::handleGetItemLinksByLinkTypeId(
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid link type ID");
            return;
        }
    }

    if (linkTypeId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid link type ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении связей для типа " << linkTypeId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void ItemLinksHandler::handleCreateItemLink(
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
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "linkTypeId is required");
                        return;
                    }

                    if (!itemLink.sourceItemId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "sourceItemId is required");
                        return;
                    }

                    if (!itemLink.destinationItemId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "destinationItemId is required");
                        return;
                    }

                    auto created = m_itemLinkService->createItemLink(itemLink, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create item link: insufficient permissions or duplicate link"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал связь элементов id=" << *created->id
                        << " (type=" << *created->linkTypeId
                        << ", source=" << *created->sourceItemId
                        << ", dest=" << *created->destinationItemId << ")";

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании связи элементов: " << e.what();
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

void ItemLinksHandler::handleDeleteItemLink(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item link ID");
        return;
    }

    LOG_DEBUG << "DELETE /item-links/" << id << " from user " << userId;

    try
    {
        auto result = m_itemLinkService->deleteItemLink(id, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил связь элементов id=" << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении связи элементов " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace server::handlers
