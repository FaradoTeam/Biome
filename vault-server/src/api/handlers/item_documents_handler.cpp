#include <regex>

#include <cpprest/uri.h>

#include "common/dto/item_document.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "item_documents_handler.h"

namespace server
{
namespace handlers
{

ItemDocumentsHandler::ItemDocumentsHandler(
    std::shared_ptr<services::IItemDocumentService> itemDocumentService
)
    : m_itemDocumentService(std::move(itemDocumentService))
{
    if (!m_itemDocumentService)
    {
        LOG_WARN << "ItemDocumentsHandler инициализирован без ItemDocumentService";
    }
}

// ============================================================
// GET /item-documents
// ============================================================

void ItemDocumentsHandler::handleGetItemDocuments(
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
            LOG_WARN << "handleGetItemDocuments: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetItemDocuments: неверный параметр pageSize: " << params["pageSize"];
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
            LOG_WARN << "handleGetItemDocuments: неверный параметр itemId: " << params["itemId"];
        }
    }

    std::optional<int64_t> documentId = std::nullopt;
    if (params.count("documentId"))
    {
        try
        {
            documentId = std::stoll(params["documentId"]);
            if (documentId <= 0)
                documentId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetItemDocuments: неверный параметр documentId: " << params["documentId"];
        }
    }

    LOG_DEBUG
        << "GET /item-documents: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", itemId=" << (itemId.has_value() ? std::to_string(*itemId) : "none")
        << ", documentId=" << (documentId.has_value() ? std::to_string(*documentId) : "none");

    try
    {
        auto pageData = m_itemDocumentService->getItemDocuments(
            page,
            pageSize,
            userId,
            itemId,
            documentId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.items.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.items[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка связей ItemDocument: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /item-documents/{id}
// ============================================================

void ItemDocumentsHandler::handleGetItemDocument(
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

    LOG_DEBUG << "GET /item-documents/" << id << " from user " << userId;

    try
    {
        auto itemDocument = m_itemDocumentService->getItemDocument(id, userId);
        if (!itemDocument)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "ItemDocument not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(itemDocument->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ItemDocument " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /items/{itemId}/documents
// ============================================================

void ItemDocumentsHandler::handleGetDocumentsByItem(
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

    // Извлекаем itemId из пути: /items/{itemId}/documents
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/documents)");
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

    LOG_DEBUG << "GET /items/" << itemId << "/documents from user " << userId;

    try
    {
        auto links = m_itemDocumentService->getDocumentsByItem(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении документов для элемента " << itemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /documents/{documentId}/items
// ============================================================

void ItemDocumentsHandler::handleGetItemsByDocument(
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

    // Извлекаем documentId из пути: /documents/{documentId}/items
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/documents/(\d+)/items)");
    std::smatch matches;

    int64_t documentId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            documentId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid document ID");
            request.reply(resp);
            return;
        }
    }

    if (documentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid document ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /documents/" << documentId << "/items from user " << userId;

    try
    {
        auto links = m_itemDocumentService->getItemsByDocument(documentId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элементов для документа " << documentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /item-documents
// ============================================================

void ItemDocumentsHandler::handleCreateItemDocument(
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

    LOG_DEBUG << "POST /item-documents from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ItemDocument itemDocument(nlohmannJson);

                    // Валидация обязательных полей
                    if (!itemDocument.itemId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "itemId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!itemDocument.documentId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "documentId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_itemDocumentService->createItemDocument(itemDocument, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create item-document link: insufficient permissions or duplicate link"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал связь ItemDocument id=" << *created->id
                        << ", itemId=" << *created->itemId
                        << ", documentId=" << *created->documentId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ItemDocument: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /item-documents/{id}
// ============================================================

void ItemDocumentsHandler::handleDeleteItemDocument(
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

    LOG_DEBUG << "DELETE /item-documents/" << id << " from user " << userId;

    try
    {
        auto result = m_itemDocumentService->deleteItemDocument(id, userId);
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
            << " удалил связь ItemDocument id=" << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ItemDocument " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
