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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(documentsPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка документов: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid document ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /documents/" << documentId << " from user " << userId;

    try
    {
        auto document = m_documentService->getDocument(documentId, userId);
        if (!document)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Document not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(document->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении документа " << documentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Document caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!document.path.has_value() || document.path->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "File path is required");
                        request.reply(resp);
                        return;
                    }

                    if (!document.filename.has_value() || document.filename->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Filename is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_documentService->createDocument(document, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create document: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " загрузил документ id=" << *created->id
                        << ", filename=" << *created->filename;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании документа: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid document ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /documents/" << documentId << "/download from user " << userId;

    try
    {
        auto document = m_documentService->getDocument(documentId, userId);
        if (!document)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Document not found");
            request.reply(resp);
            return;
        }

        if (!document->path.has_value())
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Document file not found");
            request.reply(resp);
            return;
        }

        // Читаем файл и отправляем
        std::ifstream file(*document->path, std::ios::binary);
        if (!file.is_open())
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "File not found on server");
            request.reply(resp);
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

            // Устанавливаем тело ответа - используем set_body с vector<char>
            response.set_body(
                std::vector<unsigned char>(
                    reinterpret_cast<unsigned char*>(buffer.data()),
                    reinterpret_cast<unsigned char*>(buffer.data() + buffer.size())
                )
            );

            // Устанавливаем заголовки - используем utility::conversions
            response.headers().set_content_type(
                utility::conversions::to_string_t(mimeType)
            );

            // Content-Disposition для скачивания
            std::string disposition = "attachment; filename=\"" + *document->filename + "\"";
            response.headers().add(
                U("Content-Disposition"),
                utility::conversions::to_string_t(disposition)
            );

            request.reply(response);
        }
        else
        {
            web::http::http_response resp(web::http::status_codes::InternalError);
            sendErrorResponse(resp, 500, "Failed to read file");
            request.reply(resp);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при скачивании документа " << documentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid document ID");
        request.reply(resp);
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

                    // Не позволяем менять uploadedByUserId и uploadedAt
                    // Они должны остаться из существующего документа

                    auto updated = m_documentService->updateDocument(document, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или документ не найден
                        auto existing = m_documentService->getDocument(documentId, userId);
                        if (!existing)
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Document not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this document");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил документ " << documentId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении документа " << documentId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t documentId = extractIdFromPath(request);
    if (documentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid document ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /documents/" << documentId << " from user " << userId;

    try
    {
        auto result = m_documentService->deleteDocument(documentId, userId);
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
            << " удалил документ " << documentId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении документа " << documentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
