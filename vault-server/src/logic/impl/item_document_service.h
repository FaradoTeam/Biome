#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/idocument_service.h"
#include "logic/iitem_document_service.h"
#include "logic/iitem_service.h"
#include "repo/item_document_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления связями элементов и документов.
 */
class ItemDocumentService final : public IItemDocumentService
{
public:
    ItemDocumentService(
        std::shared_ptr<repositories::IItemDocumentRepository> itemDocumentRepo,
        std::shared_ptr<IItemService> itemService,
        std::shared_ptr<IDocumentService> documentService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IItemDocumentService
    ItemDocumentsPage getItemDocuments(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> documentId = std::nullopt
    ) override;

    std::optional<dto::ItemDocument> getItemDocument(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::ItemDocument> getDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) override;

    std::vector<dto::ItemDocument> getItemsByDocument(
        int64_t documentId,
        int64_t userId
    ) override;

    std::optional<dto::ItemDocument> createItemDocument(
        const dto::ItemDocument& itemDocument,
        int64_t userId
    ) override;

    ItemDocumentResult deleteItemDocument(
        int64_t id,
        int64_t userId
    ) override;

    int64_t deleteItemDocumentsByItem(
        int64_t itemId,
        int64_t userId
    ) override;

    int64_t deleteItemDocumentsByDocument(
        int64_t documentId,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к связи.
     */
    std::optional<dto::ItemDocument> checkItemDocumentAccess(
        int64_t id,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Проверяет, может ли пользователь привязать документ к элементу.
     */
    bool canLinkDocumentToItem(
        int64_t itemId,
        int64_t documentId,
        int64_t userId
    );

private:
    std::shared_ptr<repositories::IItemDocumentRepository> m_itemDocumentRepo;
    std::shared_ptr<IItemService> m_itemService;
    std::shared_ptr<IDocumentService> m_documentService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
