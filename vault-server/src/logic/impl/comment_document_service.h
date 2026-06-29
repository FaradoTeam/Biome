#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/icomment_document_service.h"
#include "logic/icomment_service.h"
#include "logic/idocument_service.h"
#include "repo/comment_document_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления связями комментариев и документов.
 */
class CommentDocumentService final : public ICommentDocumentService
{
public:
    CommentDocumentService(
        std::shared_ptr<repositories::ICommentDocumentRepository> commentDocumentRepo,
        std::shared_ptr<ICommentService> commentService,
        std::shared_ptr<IDocumentService> documentService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // ICommentDocumentService
    CommentDocumentsPage getCommentDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> commentId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override;

    std::optional<dto::CommentDocument> getCommentDocument(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::CommentDocument> getDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) override;

    std::vector<dto::CommentDocument> getCommentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override;

    std::optional<dto::CommentDocument> createCommentDocument(
        const dto::CommentDocument& commentDocument,
        int64_t userId
    ) override;

    CommentDocumentResult deleteCommentDocument(
        int64_t id,
        int64_t userId
    ) override;

    int64_t deleteCommentDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) override;

    int64_t deleteCommentDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к связи.
     */
    std::optional<dto::CommentDocument> checkCommentDocumentAccess(
        int64_t id,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Проверяет, может ли пользователь привязать документ к комментарию.
     */
    bool canLinkDocumentToComment(
        int64_t commentId,
        int64_t documentId,
        int64_t userId
    );

private:
    std::shared_ptr<repositories::ICommentDocumentRepository> m_commentDocumentRepo;
    std::shared_ptr<ICommentService> m_commentService;
    std::shared_ptr<IDocumentService> m_documentService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
