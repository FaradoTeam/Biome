#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/icomment_document_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы со связями комментариев и документов.
 */
class CommentDocumentsHandler final : public BaseHandler
{
public:
    explicit CommentDocumentsHandler(
        std::shared_ptr<services::ICommentDocumentService> commentDocumentService
    );

    /**
     * @brief Получает список связей с пагинацией.
     * GET /comment-documents
     */
    void handleGetCommentDocuments(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает связь по ID.
     * GET /comment-documents/{id}
     */
    void handleGetCommentDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую связь комментария с документом.
     * POST /comment-documents
     */
    void handleCreateCommentDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет связь комментария с документом.
     * DELETE /comment-documents/{id}
     */
    void handleDeleteCommentDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все документы для комментария.
     * GET /comments/{commentId}/documents
     */
    void handleGetDocumentsByComment(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все комментарии для документа.
     * GET /documents/{documentId}/comments
     */
    void handleGetCommentsByDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::ICommentDocumentService> m_commentDocumentService;
};

} // namespace handlers
} // namespace server
