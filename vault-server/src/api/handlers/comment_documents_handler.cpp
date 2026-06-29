#include <regex>

#include <cpprest/uri.h>

#include "common/dto/comment_document.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "comment_documents_handler.h"

namespace server
{
namespace handlers
{

CommentDocumentsHandler::CommentDocumentsHandler(
    std::shared_ptr<services::ICommentDocumentService> commentDocumentService
)
    : m_commentDocumentService(std::move(commentDocumentService))
{
    if (!m_commentDocumentService)
    {
        LOG_WARN << "CommentDocumentsHandler инициализирован без CommentDocumentService";
    }
}

// ============================================================
// GET /comment-documents
// ============================================================

void CommentDocumentsHandler::handleGetCommentDocuments(
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
            LOG_WARN << "handleGetCommentDocuments: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetCommentDocuments: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> commentId = std::nullopt;
    if (params.count("commentId"))
    {
        try
        {
            commentId = std::stoll(params["commentId"]);
            if (commentId <= 0)
                commentId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetCommentDocuments: неверный параметр commentId: " << params["commentId"];
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
            LOG_WARN << "handleGetCommentDocuments: неверный параметр documentId: " << params["documentId"];
        }
    }

    LOG_DEBUG
        << "GET /comment-documents: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", commentId=" << (commentId.has_value() ? std::to_string(*commentId) : "none")
        << ", documentId=" << (documentId.has_value() ? std::to_string(*documentId) : "none");

    try
    {
        auto pageData = m_commentDocumentService->getCommentDocuments(
            page,
            pageSize,
            userId,
            commentId,
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
        LOG_ERROR << "Ошибка при получении списка CommentDocument: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /comment-documents/{id}
// ============================================================

void CommentDocumentsHandler::handleGetCommentDocument(
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

    LOG_DEBUG << "GET /comment-documents/" << id << " from user " << userId;

    try
    {
        auto commentDocument = m_commentDocumentService->getCommentDocument(id, userId);
        if (!commentDocument)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "CommentDocument not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(commentDocument->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении CommentDocument " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /comments/{commentId}/documents
// ============================================================

void CommentDocumentsHandler::handleGetDocumentsByComment(
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

    // Извлекаем commentId из пути: /comments/{commentId}/documents
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/comments/(\d+)/documents)");
    std::smatch matches;

    int64_t commentId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            commentId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid comment ID");
            request.reply(resp);
            return;
        }
    }

    if (commentId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid comment ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /comments/" << commentId << "/documents from user " << userId;

    try
    {
        auto links = m_commentDocumentService->getDocumentsByComment(commentId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении документов для комментария " << commentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /documents/{documentId}/comments
// ============================================================

void CommentDocumentsHandler::handleGetCommentsByDocument(
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

    // Извлекаем documentId из пути: /documents/{documentId}/comments
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/documents/(\d+)/comments)");
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

    LOG_DEBUG << "GET /documents/" << documentId << "/comments from user " << userId;

    try
    {
        auto links = m_commentDocumentService->getCommentsByDocument(documentId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < links.size(); ++i)
        {
            response[i] = dto::toWebJson(links[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении комментариев для документа " << documentId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /comment-documents
// ============================================================

void CommentDocumentsHandler::handleCreateCommentDocument(
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

    LOG_DEBUG << "POST /comment-documents from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::CommentDocument commentDocument(nlohmannJson);

                    // Валидация обязательных полей
                    if (!commentDocument.commentId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "commentId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!commentDocument.documentId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "documentId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_commentDocumentService->createCommentDocument(commentDocument, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create comment-document link: insufficient permissions or duplicate link"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал связь CommentDocument id=" << *created->id
                        << ", commentId=" << *created->commentId
                        << ", documentId=" << *created->documentId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании CommentDocument: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /comment-documents/{id}
// ============================================================

void CommentDocumentsHandler::handleDeleteCommentDocument(
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

    LOG_DEBUG << "DELETE /comment-documents/" << id << " from user " << userId;

    try
    {
        auto result = m_commentDocumentService->deleteCommentDocument(id, userId);
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
            << " удалил связь CommentDocument id=" << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении CommentDocument " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
