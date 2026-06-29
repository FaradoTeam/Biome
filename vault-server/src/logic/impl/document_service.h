#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/idocument_service.h"
#include "repo/document_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления документами.
 */
class DocumentService final : public IDocumentService
{
public:
    DocumentService(
        std::shared_ptr<repositories::IDocumentRepository> documentRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IDocumentService
    DocumentsPage getDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> uploadedByUserId = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::Document> getDocument(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::Document> createDocument(
        const dto::Document& document,
        int64_t userId
    ) override;

    std::optional<dto::Document> updateDocument(
        const dto::Document& document,
        int64_t userId
    ) override;

    DocumentResult deleteDocument(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::Document> checkDocumentAccess(
        int64_t documentId,
        int64_t userId,
        bool needWrite = false
    ) override;

private:
    /**
     * @brief Валидирует DTO документа.
     */
    bool validateDocument(
        const dto::Document& document,
        std::string& errorMessage
    );

    /**
     * @brief Проверяет, имеет ли пользователь доступ к документу.
     * @param document DTO документа
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return true если доступ разрешён
     */
    bool checkDocumentPermissions(
        const dto::Document& document,
        int64_t userId,
        bool needWrite = false
    );

private:
    std::shared_ptr<repositories::IDocumentRepository> m_documentRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
