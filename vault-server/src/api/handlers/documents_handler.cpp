#include <fstream>
#include <regex>

#include <cpprest/uri.h>

#include "common/dto/document.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "documents_handler.h"

namespace server
{
namespace handlers
{

DocumentsHandler::DocumentsHandler(
    std::shared_ptr<services::IDocumentService> documentService
)
    : m_documentService(std::move(documentService))
{
    if (!m_documentService)
    {
        LOG_WARN << "DocumentsHandler инициализирован без DocumentService";
    }
}

// ============================================================
// GET /documents
// ============================================================

void DocumentsHandler::handleGetDocuments(
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
            LOG_WARN << "handleGetDocuments: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetDocuments: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> uploadedByUserId = std::nullopt;
    if (params.count("uploadedByUserId"))
    {
        try
        {
            uploadedByUserId = std::stoll(params["uploadedByUserId"]);
            if (uploadedByUserId <= 0)
                uploadedByUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetDocuments: неверный параметр uploadedByUserId: " << params["uploadedByUserId"];
        }
    }

    std::string searchCaption;
    if (params.count("caption"))
    {
        searchCaption = params["caption"];
    }

    LOG_DEBUG
        << "GET /documents: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", uploadedByUserId=" << (uploadedByUserId.has_value() ? std::to_string(*uploadedByUserId) : "none")
        << ", searchCaption=" << searchCaption;

    try
    {
        auto documentsPage = m_documentService->getDocuments(
            page,
            pageSize,
            userId,
            uploadedByUserId,
            searchCaption
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < documentsPage.documents.size(); ++i)
        {
            items[i] = dto::toWebJson(documentsPage.documents[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(documentsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка документов: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /documents/{id}
// ============================================================

void DocumentsHandler::handleGetDocument(
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

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
        return;
    }

    LOG_DEBUG << "GET /documents/" << documentId << " from user " << userId;

    try
    {
        auto document = m_documentService->getDocument(documentId, userId);
        if (!document)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Document not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(document->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении документа " << documentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /documents (загрузка файла)
// ============================================================

void DocumentsHandler::handleCreateDocument(
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

    LOG_DEBUG << "POST /documents from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    dto::Document document(nlohmannJson);

                    // Устанавливаем загрузившего пользователя
                    document.uploadedByUserId = userId;

                    // Валидация обязательных полей
                    if (!document.caption.has_value() || document.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Document caption is required");
                        return;
                    }

                    if (!document.path.has_value() || document.path->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "File path is required");
                        return;
                    }

                    if (!document.filename.has_value() || document.filename->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Filename is required");
                        return;
                    }

                    auto created = m_documentService->createDocument(document, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create document: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " загрузил документ id=" << *created->id
                        << ", filename=" << *created->filename;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании документа: " << e.what();
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
// GET /documents/{id}/download
// ============================================================

void DocumentsHandler::handleDownloadDocument(
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

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
        return;
    }

    LOG_DEBUG << "GET /documents/" << documentId << "/download from user " << userId;

    try
    {
        auto document = m_documentService->getDocument(documentId, userId);
        if (!document)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Document not found");
            return;
        }

        if (!document->path.has_value())
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Document file not found");
            return;
        }

        // Читаем файл и отправляем
        std::ifstream file(*document->path, std::ios::binary);
        if (!file.is_open())
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "File not found on server");
            return;
        }

        // Определяем MIME-тип
        std::string mimeType = document->mimeType.value_or("application/octet-stream");

        // Читаем файл в вектор
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (file.read(buffer.data(), size))
        {
            // Создаём ответ с бинарным содержимым
            web::http::http_response response(web::http::status_codes::OK);

            // Устанавливаем тело ответа
            response.set_body(
                std::vector<unsigned char>(
                    reinterpret_cast<unsigned char*>(buffer.data()),
                    reinterpret_cast<unsigned char*>(buffer.data() + buffer.size())
                )
            );

            // Устанавливаем заголовки
            response.headers().set_content_type(
                utility::conversions::to_string_t(mimeType)
            );

            // Content-Disposition для скачивания
            std::string disposition = "attachment; filename=\"" + *document->filename + "\"";
            response.headers().add(
                U("Content-Disposition"),
                utility::conversions::to_string_t(disposition)
            );

            sendResponse(request, response);
        }
        else
        {
            sendErrorResponse(request, web::http::status_codes::InternalError, "Failed to read file");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при скачивании документа " << documentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// PUT /documents/{id}
// ============================================================

void DocumentsHandler::handleUpdateDocument(
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

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
        return;
    }

    LOG_DEBUG << "PUT /documents/" << documentId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, documentId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = documentId;
                    dto::Document document(nlohmannJson);

                    auto updated = m_documentService->updateDocument(document, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или документ не найден
                        auto existing = m_documentService->getDocument(documentId, userId);
                        if (!existing)
                        {
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Document not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this document"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил документ " << documentId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении документа " << documentId << ": " << e.what();
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
// DELETE /documents/{id}
// ============================================================

void DocumentsHandler::handleDeleteDocument(
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

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid document ID");
        return;
    }

    LOG_DEBUG << "DELETE /documents/" << documentId << " from user " << userId;

    try
    {
        auto result = m_documentService->deleteDocument(documentId, userId);
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
            << " удалил документ " << documentId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении документа " << documentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
