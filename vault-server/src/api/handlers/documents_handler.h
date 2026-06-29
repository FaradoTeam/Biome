#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/idocument_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с документами.
 */
class DocumentsHandler final : public BaseHandler
{
public:
    explicit DocumentsHandler(std::shared_ptr<services::IDocumentService> documentService);

    /**
     * @brief Получает список документов с пагинацией.
     * GET /documents
     */
    void handleGetDocuments(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает документ по ID.
     * GET /documents/{id}
     */
    void handleGetDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новый документ (загрузка файла).
     * POST /documents
     */
    void handleCreateDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Скачивает документ.
     * GET /documents/{id}/download
     */
    void handleDownloadDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет метаданные документа.
     * PUT /documents/{id}
     */
    void handleUpdateDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет документ.
     * DELETE /documents/{id}
     */
    void handleDeleteDocument(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IDocumentService> m_documentService;
};

} // namespace handlers
} // namespace server
