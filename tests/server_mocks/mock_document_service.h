#pragma once

#include <optional>
#include <string>

#include "logic/idocument_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-класс для IDocumentService.
 * Используется в юнит-тестах для имитации поведения сервиса документов.
 */
class MockDocumentService : public services::IDocumentService
{
public:
    // ============================================================
    // Методы для настройки поведения мока
    // ============================================================

    void setGetDocumentsResult(const services::DocumentsPage& result)
    {
        m_getDocumentsResult = result;
    }

    void setGetDocumentResult(const std::optional<dto::Document>& result)
    {
        m_getDocumentResult = result;
    }

    void setCreateDocumentResult(const std::optional<dto::Document>& result)
    {
        m_createDocumentResult = result;
    }

    void setUpdateDocumentResult(const std::optional<dto::Document>& result)
    {
        m_updateDocumentResult = result;
    }

    void setDeleteDocumentResult(const services::DocumentResult& result)
    {
        m_deleteDocumentResult = result;
    }

    void setCheckDocumentAccessResult(const std::optional<dto::Document>& result)
    {
        m_checkDocumentAccessResult = result;
    }

    // ============================================================
    // Геттеры для проверки вызовов
    // ============================================================

    int getGetDocumentsCallCount() const { return m_getDocumentsCallCount; }
    int getGetDocumentCallCount() const { return m_getDocumentCallCount; }
    int getCreateDocumentCallCount() const { return m_createDocumentCallCount; }
    int getUpdateDocumentCallCount() const { return m_updateDocumentCallCount; }
    int getDeleteDocumentCallCount() const { return m_deleteDocumentCallCount; }
    int getCheckDocumentAccessCallCount() const { return m_checkDocumentAccessCallCount; }

    // Параметры вызовов
    int64_t getLastGetDocumentsUserId() const { return m_lastGetDocumentsUserId; }
    int64_t getLastGetDocumentsPage() const { return m_lastGetDocumentsPage; }
    int64_t getLastGetDocumentsPageSize() const { return m_lastGetDocumentsPageSize; }
    std::optional<int64_t> getLastGetDocumentsUploadedByUserId() const
    {
        return m_lastGetDocumentsUploadedByUserId;
    }
    std::string getLastGetDocumentsSearchCaption() const
    {
        return m_lastGetDocumentsSearchCaption;
    }

    int64_t getLastGetDocumentId() const { return m_lastGetDocumentId; }
    int64_t getLastCreateDocumentUserId() const { return m_lastCreateDocumentUserId; }
    int64_t getLastUpdateDocumentId() const { return m_lastUpdateDocumentId; }
    int64_t getLastDeletedDocumentId() const { return m_lastDeletedDocumentId; }
    int64_t getLastDeleteDocumentUserId() const { return m_lastDeleteDocumentUserId; }
    int64_t getLastCheckDocumentAccessId() const { return m_lastCheckDocumentAccessId; }
    bool getLastCheckDocumentAccessNeedWrite() const { return m_lastCheckDocumentAccessNeedWrite; }

    // ============================================================
    // Вспомогательный метод для создания тестового документа
    // ============================================================

    static dto::Document createTestDocument(
        int64_t id,
        const std::string& caption,
        const std::string& path,
        const std::string& filename,
        int64_t size,
        int64_t uploadedByUserId
    )
    {
        dto::Document doc;
        doc.id = id;
        doc.caption = caption;
        doc.path = path;
        doc.filename = filename;
        doc.size = size;
        doc.uploadedByUserId = uploadedByUserId;
        doc.mimeType = "application/octet-stream";
        doc.uploadedAt = std::chrono::system_clock::now();
        return doc;
    }

    // ============================================================
    // Реализация IDocumentService
    // ============================================================

    services::DocumentsPage getDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> uploadedByUserId = std::nullopt,
        const std::string& searchCaption = ""
    ) override
    {
        m_getDocumentsCallCount++;
        m_lastGetDocumentsUserId = userId;
        m_lastGetDocumentsPage = page;
        m_lastGetDocumentsPageSize = pageSize;
        m_lastGetDocumentsUploadedByUserId = uploadedByUserId;
        m_lastGetDocumentsSearchCaption = searchCaption;
        return m_getDocumentsResult;
    }

    std::optional<dto::Document> getDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_getDocumentCallCount++;
        m_lastGetDocumentId = id;
        return m_getDocumentResult;
    }

    std::optional<dto::Document> createDocument(
        const dto::Document& document,
        int64_t userId
    ) override
    {
        m_createDocumentCallCount++;
        m_lastCreateDocumentUserId = userId;
        return m_createDocumentResult;
    }

    std::optional<dto::Document> updateDocument(
        const dto::Document& document,
        int64_t userId
    ) override
    {
        m_updateDocumentCallCount++;
        if (document.id.has_value())
        {
            m_lastUpdateDocumentId = *document.id;
        }
        return m_updateDocumentResult;
    }

    services::DocumentResult deleteDocument(
        int64_t id,
        int64_t userId
    ) override
    {
        m_deleteDocumentCallCount++;
        m_lastDeletedDocumentId = id;
        m_lastDeleteDocumentUserId = userId;
        return m_deleteDocumentResult;
    }

    std::optional<dto::Document> checkDocumentAccess(
        int64_t documentId,
        int64_t userId,
        bool needWrite = false
    ) override
    {
        m_checkDocumentAccessCallCount++;
        m_lastCheckDocumentAccessId = documentId;
        m_lastCheckDocumentAccessNeedWrite = needWrite;
        return m_checkDocumentAccessResult;
    }

private:
    // Результаты для возврата
    services::DocumentsPage m_getDocumentsResult;
    std::optional<dto::Document> m_getDocumentResult;
    std::optional<dto::Document> m_createDocumentResult;
    std::optional<dto::Document> m_updateDocumentResult;
    services::DocumentResult m_deleteDocumentResult;
    std::optional<dto::Document> m_checkDocumentAccessResult;

    // Счётчики вызовов
    int m_getDocumentsCallCount = 0;
    int m_getDocumentCallCount = 0;
    int m_createDocumentCallCount = 0;
    int m_updateDocumentCallCount = 0;
    int m_deleteDocumentCallCount = 0;
    int m_checkDocumentAccessCallCount = 0;

    // Параметры вызовов
    int64_t m_lastGetDocumentsUserId = 0;
    int64_t m_lastGetDocumentsPage = 0;
    int64_t m_lastGetDocumentsPageSize = 0;
    std::optional<int64_t> m_lastGetDocumentsUploadedByUserId;
    std::string m_lastGetDocumentsSearchCaption;

    int64_t m_lastGetDocumentId = 0;
    int64_t m_lastCreateDocumentUserId = 0;
    int64_t m_lastUpdateDocumentId = 0;
    int64_t m_lastDeletedDocumentId = 0;
    int64_t m_lastDeleteDocumentUserId = 0;
    int64_t m_lastCheckDocumentAccessId = 0;
    bool m_lastCheckDocumentAccessNeedWrite = false;
};

} // namespace tests
} // namespace server
