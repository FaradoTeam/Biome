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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка связей ItemDocument: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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

    LOG_DEBUG << "GET /item-documents/" << id << " from user " << userId;

    try
    {
        auto itemDocument = m_itemDocumentService->getItemDocument(id, userId);
        if (!itemDocument)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "ItemDocument not found");
            return;
        }

        sendJsonResponse(
            request,
            web::http::status_codes::OK,
            dto::toWebJson(itemDocument->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении ItemDocument " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении документов для элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
            return;
        }
    }

    if (documentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элементов для документа " << documentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "itemId is required");
                        return;
                    }

                    if (!itemDocument.documentId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "documentId is required");
                        return;
                    }

                    auto created = m_itemDocumentService->createItemDocument(itemDocument, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create item-document link: insufficient permissions or duplicate link"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал связь ItemDocument id=" << *created->id
                        << ", itemId=" << *created->itemId
                        << ", documentId=" << *created->documentId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании ItemDocument: " << e.what();
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
// DELETE /item-documents/{id}
// ============================================================

void ItemDocumentsHandler::handleDeleteItemDocument(
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

    LOG_DEBUG << "DELETE /item-documents/" << id << " from user " << userId;

    try
    {
        auto result = m_itemDocumentService->deleteItemDocument(id, userId);
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
            << " удалил связь ItemDocument id=" << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении ItemDocument " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
