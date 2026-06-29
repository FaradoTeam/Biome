#pragma once

#include <optional>
#include <vector>

#include "logic/iitem_document_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для IItemDocumentService.
 * Используется в юнит-тестах для имитации поведения сервиса связей элементов и документов.
 */
class MockItemDocumentService : public services::IItemDocumentService
{
public:
    // ============================================================
    // Методы для настройки поведения мока
    // ============================================================

    void setGetItemDocumentsResult(const services::ItemDocumentsPage& result)
    {
        m_getItemDocumentsResult = result;
    }

    void setGetItemDocumentResult(const std::optional<dto::ItemDocument>& result)
    {
        m_getItemDocumentResult = result;
    }

    void setGetDocumentsByItemResult(const std::vector<dto::ItemDocument>& result)
    {
        m_getDocumentsByItemResult = result;
    }

    void setGetItemsByDocumentResult(const std::vector<dto::ItemDocument>& result)
    {
        m_getItemsByDocumentResult = result;
    }

    void setCreateItemDocumentResult(const std::optional<dto::ItemDocument>& result)
    {
        m_createItemDocumentResult = result;
    }

    void setDeleteItemDocumentResult(const services::ItemDocumentResult& result)
    {
        m_deleteItemDocumentResult = result;
    }

    void setDeleteItemDocumentsByItemResult(int64_t result)
    {
        m_deleteItemDocumentsByItemResult = result;
    }

    void setDeleteItemDocumentsByDocumentResult(int64_t result)
    {
        m_deleteItemDocumentsByDocumentResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetItemDocumentsCallCount() const { return m_getItemDocumentsCallCount; }
    int getGetItemDocumentCallCount() const { return m_getItemDocumentCallCount; }
    int getGetDocumentsByItemCallCount() const { return m_getDocumentsByItemCallCount; }
    int getGetItemsByDocumentCallCount() const { return m_getItemsByDocumentCallCount; }
    int getCreateItemDocumentCallCount() const { return m_createItemDocumentCallCount; }
    int getDeleteItemDocumentCallCount() const { return m_deleteItemDocumentCallCount; }
    int getDeleteItemDocumentsByItemCallCount() const { return m_deleteItemDocumentsByItemCallCount; }
    int getDeleteItemDocumentsByDocumentCallCount() const { return m_deleteItemDocumentsByDocumentCallCount; }

    // Параметры вызовов
    int64_t getLastGetItemDocumentsUserId() const { return m_lastGetItemDocumentsUserId; }
    int64_t getLastGetItemDocumentsPage() const { return m_lastGetItemDocumentsPage; }
    int64_t getLastGetItemDocumentsPageSize() const { return m_lastGetItemDocumentsPageSize; }
    std::optional<int64_t> getLastGetItemDocumentsItemId() const
    {
        return m_lastGetItemDocumentsItemId;
    }
    std::optional<int64_t> getLastGetItemDocumentsDocumentId() const
    {
        return m_lastGetItemDocumentsDocumentId;
    }

    int64_t getLastGetItemDocumentId() const { return m_lastGetItemDocumentId; }
    int64_t getLastGetDocumentsByItemId() const { return m_lastGetDocumentsByItemId; }
    int64_t getLastGetItemsByDocumentId() const { return m_lastGetItemsByDocumentId; }
    int64_t getLastCreateItemDocumentUserId() const { return m_lastCreateItemDocumentUserId; }
    int64_t getLastDeletedItemDocumentId() const { return m_lastDeletedItemDocumentId; }
    int64_t getLastDeleteItemDocumentUserId() const { return m_lastDeleteItemDocumentUserId; }
    int64_t getLastDeleteItemDocumentsByItemId() const { return m_lastDeleteItemDocumentsByItemId; }
    int64_t getLastDeleteItemDocumentsByDocumentId() const { return m_lastDeleteItemDocumentsByDocumentId; }

    // ============================================================
    // Вспомогательный метод для создания тестовой связи
    // ============================================================

    static dto::ItemDocument createTestItemDocument(
        int64_t id,
        int64_t itemId,
        int64_t documentId
    )
    {
        dto::ItemDocument link;
        link.id = id;
        link.itemId = itemId;
        link.documentId = documentId;
        return link;
    }

    // ============================================================
    // Реализация IItemDocumentService
    // ============================================================

    services::ItemDocumentsPage getItemDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override
    {
        m_getItemDocumentsCallCount++;
        m_lastGetItemDocumentsUserId = userId;
        m_lastGetItemDocumentsPage = page;
        m_lastGetItemDocumentsPageSize = pageSize;
        m_lastGetItemDocumentsItemId = itemId;
        m_lastGetItemDocumentsDocumentId = documentId;
        return m_getItemDocumentsResult;
    }

    std::optional<dto::ItemDocument> getItemDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getItemDocumentCallCount++;
        m_lastGetItemDocumentId = id;
        return m_getItemDocumentResult;
    }

    std::vector<dto::ItemDocument> getDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) override
    {
        m_getDocumentsByItemCallCount++;
        m_lastGetDocumentsByItemId = itemId;
        return m_getDocumentsByItemResult;
    }

    std::vector<dto::ItemDocument> getItemsByDocument(
        int64_t documentId,
        int64_t userId
    ) override
    {
        m_getItemsByDocumentCallCount++;
        m_lastGetItemsByDocumentId = documentId;
        return m_getItemsByDocumentResult;
    }

    std::optional<dto::ItemDocument> createItemDocument(
        const dto::ItemDocument& itemDocument,
        int64_t userId
    ) override
    {
        m_createItemDocumentCallCount++;
        m_lastCreateItemDocumentUserId = userId;
        return m_createItemDocumentResult;
    }

    services::ItemDocumentResult deleteItemDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteItemDocumentCallCount++;
        m_lastDeletedItemDocumentId = id;
        m_lastDeleteItemDocumentUserId = userId;
        return m_deleteItemDocumentResult;
    }

    int64_t deleteItemDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) override
    {
        m_deleteItemDocumentsByItemCallCount++;
        m_lastDeleteItemDocumentsByItemId = itemId;
        return m_deleteItemDocumentsByItemResult;
    }

    int64_t deleteItemDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override
    {
        m_deleteItemDocumentsByDocumentCallCount++;
        m_lastDeleteItemDocumentsByDocumentId = documentId;
        return m_deleteItemDocumentsByDocumentResult;
    }

private:
    // Результаты для возврата
    services::ItemDocumentsPage m_getItemDocumentsResult;
    std::optional<dto::ItemDocument> m_getItemDocumentResult;
    std::vector<dto::ItemDocument> m_getDocumentsByItemResult;
    std::vector<dto::ItemDocument> m_getItemsByDocumentResult;
    std::optional<dto::ItemDocument> m_createItemDocumentResult;
    services::ItemDocumentResult m_deleteItemDocumentResult;
    int64_t m_deleteItemDocumentsByItemResult = 0;
    int64_t m_deleteItemDocumentsByDocumentResult = 0;

    // Счётчики вызовов
    int m_getItemDocumentsCallCount = 0;
    int m_getItemDocumentCallCount = 0;
    int m_getDocumentsByItemCallCount = 0;
    int m_getItemsByDocumentCallCount = 0;
    int m_createItemDocumentCallCount = 0;
    int m_deleteItemDocumentCallCount = 0;
    int m_deleteItemDocumentsByItemCallCount = 0;
    int m_deleteItemDocumentsByDocumentCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetItemDocumentsUserId = 0;
    int64_t m_lastGetItemDocumentsPage = 0;
    int64_t m_lastGetItemDocumentsPageSize = 0;
    std::optional<int64_t> m_lastGetItemDocumentsItemId;
    std::optional<int64_t> m_lastGetItemDocumentsDocumentId;

    int64_t m_lastGetItemDocumentId = 0;
    int64_t m_lastGetDocumentsByItemId = 0;
    int64_t m_lastGetItemsByDocumentId = 0;
    int64_t m_lastCreateItemDocumentUserId = 0;
    int64_t m_lastDeletedItemDocumentId = 0;
    int64_t m_lastDeleteItemDocumentUserId = 0;
    int64_t m_lastDeleteItemDocumentsByItemId = 0;
    int64_t m_lastDeleteItemDocumentsByDocumentId = 0;
};

} // namespace tests
} // namespace server
