#pragma once

#include <optional>
#include <vector>

#include "logic/icomment_document_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для ICommentDocumentService.
 * Используется в юнит-тестах для имитации поведения сервиса связей комментариев и документов.
 */
class MockCommentDocumentService : public services::ICommentDocumentService
{
public:
    // ============================================================
    // Методы для настройки поведения мока
    // ============================================================

    void setGetCommentDocumentsResult(const services::CommentDocumentsPage& result)
    {
        m_getCommentDocumentsResult = result;
    }

    void setGetCommentDocumentResult(const std::optional<dto::CommentDocument>& result)
    {
        m_getCommentDocumentResult = result;
    }

    void setGetDocumentsByCommentResult(const std::vector<dto::CommentDocument>& result)
    {
        m_getDocumentsByCommentResult = result;
    }

    void setGetCommentsByDocumentResult(const std::vector<dto::CommentDocument>& result)
    {
        m_getCommentsByDocumentResult = result;
    }

    void setCreateCommentDocumentResult(const std::optional<dto::CommentDocument>& result)
    {
        m_createCommentDocumentResult = result;
    }

    void setDeleteCommentDocumentResult(const services::CommentDocumentResult& result)
    {
        m_deleteCommentDocumentResult = result;
    }

    void setDeleteCommentDocumentsByCommentResult(int64_t result)
    {
        m_deleteCommentDocumentsByCommentResult = result;
    }

    void setDeleteCommentDocumentsByDocumentResult(int64_t result)
    {
        m_deleteCommentDocumentsByDocumentResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetCommentDocumentsCallCount() const { return m_getCommentDocumentsCallCount; }
    int getGetCommentDocumentCallCount() const { return m_getCommentDocumentCallCount; }
    int getGetDocumentsByCommentCallCount() const { return m_getDocumentsByCommentCallCount; }
    int getGetCommentsByDocumentCallCount() const { return m_getCommentsByDocumentCallCount; }
    int getCreateCommentDocumentCallCount() const { return m_createCommentDocumentCallCount; }
    int getDeleteCommentDocumentCallCount() const { return m_deleteCommentDocumentCallCount; }
    int getDeleteCommentDocumentsByCommentCallCount() const
    {
        return m_deleteCommentDocumentsByCommentCallCount;
    }
    int getDeleteCommentDocumentsByDocumentCallCount() const
    {
        return m_deleteCommentDocumentsByDocumentCallCount;
    }

    // Параметры вызовов
    int64_t getLastGetCommentDocumentsUserId() const { return m_lastGetCommentDocumentsUserId; }
    int64_t getLastGetCommentDocumentsPage() const { return m_lastGetCommentDocumentsPage; }
    int64_t getLastGetCommentDocumentsPageSize() const { return m_lastGetCommentDocumentsPageSize; }
    std::optional<int64_t> getLastGetCommentDocumentsCommentId() const
    {
        return m_lastGetCommentDocumentsCommentId;
    }
    std::optional<int64_t> getLastGetCommentDocumentsDocumentId() const
    {
        return m_lastGetCommentDocumentsDocumentId;
    }

    int64_t getLastGetCommentDocumentId() const { return m_lastGetCommentDocumentId; }
    int64_t getLastGetDocumentsByCommentId() const { return m_lastGetDocumentsByCommentId; }
    int64_t getLastGetCommentsByDocumentId() const { return m_lastGetCommentsByDocumentId; }
    int64_t getLastCreateCommentDocumentUserId() const { return m_lastCreateCommentDocumentUserId; }
    int64_t getLastDeletedCommentDocumentId() const { return m_lastDeletedCommentDocumentId; }
    int64_t getLastDeleteCommentDocumentUserId() const { return m_lastDeleteCommentDocumentUserId; }
    int64_t getLastDeleteCommentDocumentsByCommentId() const
    {
        return m_lastDeleteCommentDocumentsByCommentId;
    }
    int64_t getLastDeleteCommentDocumentsByDocumentId() const
    {
        return m_lastDeleteCommentDocumentsByDocumentId;
    }

    // ============================================================
    // Вспомогательный метод для создания тестовой связи
    // ============================================================

    static dto::CommentDocument createTestCommentDocument(
        int64_t id,
        int64_t commentId,
        int64_t documentId
    )
    {
        dto::CommentDocument link;
        link.id = id;
        link.commentId = commentId;
        link.documentId = documentId;
        return link;
    }

    // ============================================================
    // Реализация ICommentDocumentService
    // ============================================================

    services::CommentDocumentsPage getCommentDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> commentId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override
    {
        m_getCommentDocumentsCallCount++;
        m_lastGetCommentDocumentsUserId = userId;
        m_lastGetCommentDocumentsPage = page;
        m_lastGetCommentDocumentsPageSize = pageSize;
        m_lastGetCommentDocumentsCommentId = commentId;
        m_lastGetCommentDocumentsDocumentId = documentId;
        return m_getCommentDocumentsResult;
    }

    std::optional<dto::CommentDocument> getCommentDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getCommentDocumentCallCount++;
        m_lastGetCommentDocumentId = id;
        return m_getCommentDocumentResult;
    }

    std::vector<dto::CommentDocument> getDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) override
    {
        m_getDocumentsByCommentCallCount++;
        m_lastGetDocumentsByCommentId = commentId;
        return m_getDocumentsByCommentResult;
    }

    std::vector<dto::CommentDocument> getCommentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override
    {
        m_getCommentsByDocumentCallCount++;
        m_lastGetCommentsByDocumentId = documentId;
        return m_getCommentsByDocumentResult;
    }

    std::optional<dto::CommentDocument> createCommentDocument(
        const dto::CommentDocument& commentDocument,
        int64_t userId
    ) override
    {
        m_createCommentDocumentCallCount++;
        m_lastCreateCommentDocumentUserId = userId;
        return m_createCommentDocumentResult;
    }

    services::CommentDocumentResult deleteCommentDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteCommentDocumentCallCount++;
        m_lastDeletedCommentDocumentId = id;
        m_lastDeleteCommentDocumentUserId = userId;
        return m_deleteCommentDocumentResult;
    }

    int64_t deleteCommentDocumentsByComment(
        int64_t commentId,
        int64_t userId
    ) override
    {
        m_deleteCommentDocumentsByCommentCallCount++;
        m_lastDeleteCommentDocumentsByCommentId = commentId;
        return m_deleteCommentDocumentsByCommentResult;
    }

    int64_t deleteCommentDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override
    {
        m_deleteCommentDocumentsByDocumentCallCount++;
        m_lastDeleteCommentDocumentsByDocumentId = documentId;
        return m_deleteCommentDocumentsByDocumentResult;
    }

private:
    // Результаты для возврата
    services::CommentDocumentsPage m_getCommentDocumentsResult;
    std::optional<dto::CommentDocument> m_getCommentDocumentResult;
    std::vector<dto::CommentDocument> m_getDocumentsByCommentResult;
    std::vector<dto::CommentDocument> m_getCommentsByDocumentResult;
    std::optional<dto::CommentDocument> m_createCommentDocumentResult;
    services::CommentDocumentResult m_deleteCommentDocumentResult;
    int64_t m_deleteCommentDocumentsByCommentResult = 0;
    int64_t m_deleteCommentDocumentsByDocumentResult = 0;

    // Счётчики вызовов
    int m_getCommentDocumentsCallCount = 0;
    int m_getCommentDocumentCallCount = 0;
    int m_getDocumentsByCommentCallCount = 0;
    int m_getCommentsByDocumentCallCount = 0;
    int m_createCommentDocumentCallCount = 0;
    int m_deleteCommentDocumentCallCount = 0;
    int m_deleteCommentDocumentsByCommentCallCount = 0;
    int m_deleteCommentDocumentsByDocumentCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetCommentDocumentsUserId = 0;
    int64_t m_lastGetCommentDocumentsPage = 0;
    int64_t m_lastGetCommentDocumentsPageSize = 0;
    std::optional<int64_t> m_lastGetCommentDocumentsCommentId;
    std::optional<int64_t> m_lastGetCommentDocumentsDocumentId;

    int64_t m_lastGetCommentDocumentId = 0;
    int64_t m_lastGetDocumentsByCommentId = 0;
    int64_t m_lastGetCommentsByDocumentId = 0;
    int64_t m_lastCreateCommentDocumentUserId = 0;
    int64_t m_lastDeletedCommentDocumentId = 0;
    int64_t m_lastDeleteCommentDocumentUserId = 0;
    int64_t m_lastDeleteCommentDocumentsByCommentId = 0;
    int64_t m_lastDeleteCommentDocumentsByDocumentId = 0;
};

} // namespace tests
} // namespace server
