#include "comment_document_service.h"
#include "common/log/log.h"

namespace server
{
namespace services
{

CommentDocumentService::CommentDocumentService(
    std::shared_ptr<repositories::ICommentDocumentRepository> commentDocumentRepo,
    std::shared_ptr<ICommentService> commentService,
    std::shared_ptr<IDocumentService> documentService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_commentDocumentRepo(std::move(commentDocumentRepo))
    , m_commentService(std::move(commentService))
    , m_documentService(std::move(documentService))
    , m_authzService(std::move(authzService))
{
    if (!m_commentDocumentRepo || !m_commentService || !m_documentService || !m_authzService)
    {
        throw std::runtime_error("CommentDocumentService: один или несколько компонентов не инициализированы");
    }
}

CommentDocumentsPage CommentDocumentService::getCommentDocuments(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> commentId,
    std::optional<int64_t> documentId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан commentId, проверяем доступ к комментарию
    if (commentId.has_value() && !m_commentService->getComment(*commentId, userId).has_value())
    {
        LOG_WARN
            << "getCommentDocuments: пользователь " << userId
            << " не имеет доступа к комментарию " << *commentId;
        return { {}, 0 };
    }

    // Если указан documentId, проверяем доступ к документу
    if (documentId.has_value() && !m_documentService->checkDocumentAccess(*documentId, userId, false).has_value())
    {
        LOG_WARN
            << "getCommentDocuments: пользователь " << userId
            << " не имеет доступа к документу " << *documentId;
        return { {}, 0 };
    }

    auto [items, total] = m_commentDocumentRepo->findAll(
        page, pageSize, commentId, documentId
    );

    // Фильтруем связи по правам доступа
    std::vector<dto::CommentDocument> filtered;
    for (const auto& link : items)
    {
        if (m_commentService->getComment(*link.commentId, userId).has_value() && m_documentService->checkDocumentAccess(*link.documentId, userId, false).has_value())
        {
            filtered.push_back(link);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::CommentDocument> CommentDocumentService::getCommentDocument(
    int64_t id,
    int64_t userId
)
{
    return checkCommentDocumentAccess(id, userId, false);
}

std::vector<dto::CommentDocument> CommentDocumentService::getDocumentsByComment(
    int64_t commentId,
    int64_t userId
)
{
    if (!m_commentService->getComment(commentId, userId).has_value())
    {
        LOG_WARN
            << "getDocumentsByComment: пользователь " << userId
            << " не имеет доступа к комментарию " << commentId;
        return {};
    }

    auto links = m_commentDocumentRepo->findByCommentId(commentId);

    // Фильтруем по доступу к документам
    std::vector<dto::CommentDocument> filtered;
    for (const auto& link : links)
    {
        if (m_documentService->checkDocumentAccess(*link.documentId, userId, false).has_value())
        {
            filtered.push_back(link);
        }
    }

    return filtered;
}

std::vector<dto::CommentDocument> CommentDocumentService::getCommentsByDocument(
    int64_t documentId,
    int64_t userId
)
{
    auto document = m_documentService->checkDocumentAccess(documentId, userId, false);
    if (!document.has_value())
    {
        LOG_WARN
            << "getCommentsByDocument: пользователь " << userId
            << " не имеет доступа к документу " << documentId;
        return {};
    }

    auto links = m_commentDocumentRepo->findByDocumentId(documentId);

    // Фильтруем по доступу к комментариям
    std::vector<dto::CommentDocument> filtered;
    for (const auto& link : links)
    {
        if (m_commentService->getComment(*link.commentId, userId).has_value())
        {
            filtered.push_back(link);
        }
    }

    return filtered;
}

std::optional<dto::CommentDocument> CommentDocumentService::createCommentDocument(
    const dto::CommentDocument& commentDocument,
    int64_t userId
)
{
    if (!commentDocument.commentId.has_value() || !commentDocument.documentId.has_value())
    {
        LOG_WARN << "createCommentDocument: отсутствуют обязательные поля";
        return std::nullopt;
    }

    if (!canLinkDocumentToComment(*commentDocument.commentId, *commentDocument.documentId, userId))
    {
        LOG_WARN
            << "createCommentDocument: недостаточно прав для привязки документа "
            << *commentDocument.documentId << " к комментарию " << *commentDocument.commentId;
        return std::nullopt;
    }

    // Проверяем, не существует ли уже такой связи
    if (m_commentDocumentRepo->exists(*commentDocument.commentId, *commentDocument.documentId))
    {
        LOG_WARN
            << "createCommentDocument: связь уже существует, commentId="
            << *commentDocument.commentId << ", documentId=" << *commentDocument.documentId;
        return std::nullopt;
    }

    const int64_t newId = m_commentDocumentRepo->create(commentDocument);
    if (newId <= 0)
    {
        LOG_ERROR << "createCommentDocument: не удалось создать связь";
        return std::nullopt;
    }

    LOG_INFO
        << "Связь комментария с документом создана: id=" << newId
        << ", commentId=" << *commentDocument.commentId
        << ", documentId=" << *commentDocument.documentId
        << ", пользователь=" << userId;

    return m_commentDocumentRepo->findById(newId);
}

CommentDocumentResult CommentDocumentService::deleteCommentDocument(
    int64_t id,
    int64_t userId
)
{
    CommentDocumentResult result;

    auto link = checkCommentDocumentAccess(id, userId, true);
    if (!link.has_value())
    {
        result.errorMessage = "Связь не найдена или нет доступа";
        result.errorCode = 404;
        return result;
    }

    if (!m_commentDocumentRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить связь";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Связь комментария с документом удалена: id=" << id
        << ", пользователь=" << userId;

    return result;
}

int64_t CommentDocumentService::deleteCommentDocumentsByComment(
    int64_t commentId,
    int64_t userId
)
{
    if (!m_commentService->getComment(commentId, userId).has_value())
    {
        LOG_WARN
            << "deleteCommentDocumentsByComment: пользователь " << userId
            << " не имеет доступа к комментарию " << commentId;
        return 0;
    }

    // Проверяем право на запись к комментарию
    auto comment = m_commentService->getComment(commentId, userId);
    if (!comment.has_value())
    {
        return 0;
    }

    return m_commentDocumentRepo->removeByCommentId(commentId);
}

int64_t CommentDocumentService::deleteCommentDocumentsByDocument(
    int64_t documentId,
    int64_t userId
)
{
    auto document = m_documentService->checkDocumentAccess(documentId, userId, true);
    if (!document.has_value())
    {
        LOG_WARN
            << "deleteCommentDocumentsByDocument: пользователь " << userId
            << " не имеет доступа к документу " << documentId;
        return 0;
    }

    return m_commentDocumentRepo->removeByDocumentId(documentId);
}

std::optional<dto::CommentDocument> CommentDocumentService::checkCommentDocumentAccess(
    int64_t id,
    int64_t userId,
    bool needWrite
)
{
    auto link = m_commentDocumentRepo->findById(id);
    if (!link.has_value())
    {
        LOG_DEBUG << "checkCommentDocumentAccess: связь не найдена, id=" << id;
        return std::nullopt;
    }

    if (!m_commentService->getComment(*link->commentId, userId).has_value())
    {
        LOG_WARN
            << "checkCommentDocumentAccess: нет доступа к комментарию "
            << *link->commentId;
        return std::nullopt;
    }

    if (!m_documentService->checkDocumentAccess(*link->documentId, userId, needWrite).has_value())
    {
        LOG_WARN
            << "checkCommentDocumentAccess: нет доступа к документу "
            << *link->documentId;
        return std::nullopt;
    }

    return link;
}

bool CommentDocumentService::canLinkDocumentToComment(
    int64_t commentId,
    int64_t documentId,
    int64_t userId
)
{
    // Проверяем доступ к комментарию (нужно право на запись)
    auto comment = m_commentService->getComment(commentId, userId);
    if (!comment.has_value())
    {
        LOG_DEBUG
            << "canLinkDocumentToComment: нет доступа к комментарию " << commentId;
        return false;
    }

    // Проверяем доступ к документу (нужно право на запись)
    auto document = m_documentService->checkDocumentAccess(documentId, userId, true);
    if (!document.has_value())
    {
        LOG_DEBUG
            << "canLinkDocumentToComment: нет доступа к документу " << documentId;
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
