#pragma once

#include <optional>
#include <string>
#include <vector>

#include "logic/icomment_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для ICommentService.
 * Используется в юнит-тестах для имитации поведения сервиса комментариев.
 */
class MockCommentService : public services::ICommentService
{
public:
    // ============================================================
    // Методы для настройки поведения мока
    // ============================================================

    void setGetCommentsResult(const services::CommentsPage& result)
    {
        m_getCommentsResult = result;
    }

    void setGetCommentResult(const std::optional<dto::Comment>& result)
    {
        m_getCommentResult = result;
    }

    void setGetCommentsByItemResult(const std::vector<dto::Comment>& result)
    {
        m_getCommentsByItemResult = result;
    }

    void setCreateCommentResult(const std::optional<dto::Comment>& result)
    {
        m_createCommentResult = result;
    }

    void setUpdateCommentResult(const std::optional<dto::Comment>& result)
    {
        m_updateCommentResult = result;
    }

    void setDeleteCommentResult(const services::CommentResult& result)
    {
        m_deleteCommentResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetCommentsCallCount() const { return m_getCommentsCallCount; }
    int getGetCommentCallCount() const { return m_getCommentCallCount; }
    int getGetCommentsByItemCallCount() const { return m_getCommentsByItemCallCount; }
    int getCreateCommentCallCount() const { return m_createCommentCallCount; }
    int getUpdateCommentCallCount() const { return m_updateCommentCallCount; }
    int getDeleteCommentCallCount() const { return m_deleteCommentCallCount; }

    // Параметры вызовов
    int64_t getLastGetCommentsUserId() const { return m_lastGetCommentsUserId; }
    int64_t getLastGetCommentsPage() const { return m_lastGetCommentsPage; }
    int64_t getLastGetCommentsPageSize() const { return m_lastGetCommentsPageSize; }
    std::optional<int64_t> getLastGetCommentsItemId() const { return m_lastGetCommentsItemId; }
    std::optional<int64_t> getLastGetCommentsFilterUserId() const
    {
        return m_lastGetCommentsFilterUserId;
    }
    std::optional<common::DateTime> getLastGetCommentsDateFrom() const
    {
        return m_lastGetCommentsDateFrom;
    }
    std::optional<common::DateTime> getLastGetCommentsDateTo() const
    {
        return m_lastGetCommentsDateTo;
    }

    int64_t getLastGetCommentId() const { return m_lastGetCommentId; }
    int64_t getLastGetCommentsByItemId() const { return m_lastGetCommentsByItemId; }
    int64_t getLastCreateCommentUserId() const { return m_lastCreateCommentUserId; }
    int64_t getLastUpdateCommentId() const { return m_lastUpdateCommentId; }
    int64_t getLastDeletedCommentId() const { return m_lastDeletedCommentId; }
    int64_t getLastDeleteCommentUserId() const { return m_lastDeleteCommentUserId; }

    // ============================================================
    // Вспомогательный метод для создания тестового комментария
    // ============================================================

    static dto::Comment createTestComment(
        int64_t id,
        int64_t userId,
        int64_t itemId,
        const std::string& content
    )
    {
        dto::Comment comment;
        comment.id = id;
        comment.userId = userId;
        comment.itemId = itemId;
        comment.content = content;
        comment.createdAt = std::chrono::system_clock::now();
        return comment;
    }

    // ============================================================
    // Реализация ICommentService
    // ============================================================

    services::CommentsPage getComments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override
    {
        m_getCommentsCallCount++;
        m_lastGetCommentsUserId = userId;
        m_lastGetCommentsPage = page;
        m_lastGetCommentsPageSize = pageSize;
        m_lastGetCommentsItemId = itemId;
        m_lastGetCommentsFilterUserId = filterUserId;
        m_lastGetCommentsDateFrom = dateFrom;
        m_lastGetCommentsDateTo = dateTo;
        return m_getCommentsResult;
    }

    std::optional<dto::Comment> getComment(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getCommentCallCount++;
        m_lastGetCommentId = id;
        return m_getCommentResult;
    }

    std::vector<dto::Comment> getCommentsByItem(
        int64_t itemId,
        int64_t userId
    ) override
    {
        m_getCommentsByItemCallCount++;
        m_lastGetCommentsByItemId = itemId;
        return m_getCommentsByItemResult;
    }

    std::optional<dto::Comment> createComment(
        const dto::Comment& comment,
        int64_t userId
    ) override
    {
        m_createCommentCallCount++;
        m_lastCreateCommentUserId = userId;
        return m_createCommentResult;
    }

    std::optional<dto::Comment> updateComment(
        const dto::Comment& comment,
        int64_t userId
    ) override
    {
        m_updateCommentCallCount++;
        if (comment.id.has_value())
        {
            m_lastUpdateCommentId = *comment.id;
        }
        return m_updateCommentResult;
    }

    services::CommentResult deleteComment(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteCommentCallCount++;
        m_lastDeletedCommentId = id;
        m_lastDeleteCommentUserId = userId;
        return m_deleteCommentResult;
    }

private:
    // Результаты для возврата
    services::CommentsPage m_getCommentsResult;
    std::optional<dto::Comment> m_getCommentResult;
    std::vector<dto::Comment> m_getCommentsByItemResult;
    std::optional<dto::Comment> m_createCommentResult;
    std::optional<dto::Comment> m_updateCommentResult;
    services::CommentResult m_deleteCommentResult;

    // Счётчики вызовов
    int m_getCommentsCallCount = 0;
    int m_getCommentCallCount = 0;
    int m_getCommentsByItemCallCount = 0;
    int m_createCommentCallCount = 0;
    int m_updateCommentCallCount = 0;
    int m_deleteCommentCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetCommentsUserId = 0;
    int64_t m_lastGetCommentsPage = 0;
    int64_t m_lastGetCommentsPageSize = 0;
    std::optional<int64_t> m_lastGetCommentsItemId;
    std::optional<int64_t> m_lastGetCommentsFilterUserId;
    std::optional<common::DateTime> m_lastGetCommentsDateFrom;
    std::optional<common::DateTime> m_lastGetCommentsDateTo;

    int64_t m_lastGetCommentId = 0;
    int64_t m_lastGetCommentsByItemId = 0;
    int64_t m_lastCreateCommentUserId = 0;
    int64_t m_lastUpdateCommentId = 0;
    int64_t m_lastDeletedCommentId = 0;
    int64_t m_lastDeleteCommentUserId = 0;
};

} // namespace tests
} // namespace server
