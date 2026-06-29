#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_document_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы со связями элементов и документов.
 */
class ItemDocumentsHandler final : public BaseHandler
{
public:
    explicit ItemDocumentsHandler(
        std::shared_ptr<services::IItemDocumentService> itemDocumentService
    );

    /**
     * @brief Получает список связей с пагинацией.
     * GET /item-documents
     */
    void handleGetItemDocuments(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает связь по ID.
     * GET /item-documents/{id}
     */
    void handleGetItemDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую связь элемента с документом.
     * POST /item-documents
     */
    void handleCreateItemDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет связь элемента с документом.
     * DELETE /item-documents/{id}
     */
    void handleDeleteItemDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все документы для элемента.
     * GET /items/{itemId}/documents
     */
    void handleGetDocumentsByItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все элементы для документа.
     * GET /documents/{documentId}/items
     */
    void handleGetItemsByDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IItemDocumentService> m_itemDocumentService;
};

} // namespace handlers
} // namespace server
