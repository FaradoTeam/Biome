#include "item_document_service.h"
#include "common/log/log.h"

namespace server
{
namespace services
{

ItemDocumentService::ItemDocumentService(
    std::shared_ptr<repositories::IItemDocumentRepository> itemDocumentRepo,
    std::shared_ptr<IItemService> itemService,
    std::shared_ptr<IDocumentService> documentService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_itemDocumentRepo(std::move(itemDocumentRepo))
    , m_itemService(std::move(itemService))
    , m_documentService(std::move(documentService))
    , m_authzService(std::move(authzService))
{
    if (!m_itemDocumentRepo || !m_itemService || !m_documentService || !m_authzService)
    {
        throw std::runtime_error("ItemDocumentService: один или несколько компонентов не инициализированы");
    }
}

ItemDocumentsPage ItemDocumentService::getItemDocuments(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> itemId,
    std::optional<int64_t> documentId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Если указан itemId, проверяем доступ к элементу
    if (itemId.has_value() && !m_itemService->item(*itemId, userId).has_value())
    {
        LOG_WARN
            << "getItemDocuments: пользователь " << userId
            << " не имеет доступа к элементу " << *itemId;
        return { {}, 0 };
    }

    // Если указан documentId, проверяем доступ к документу
    if (documentId.has_value() && !m_documentService->checkDocumentAccess(*documentId, userId, false).has_value())
    {
        LOG_WARN
            << "getItemDocuments: пользователь " << userId
            << " не имеет доступа к документу " << *documentId;
        return { {}, 0 };
    }

    auto [items, total] = m_itemDocumentRepo->findAll(
        page, pageSize, itemId, documentId
    );

    // Фильтруем связи по правам доступа
    std::vector<dto::ItemDocument> filtered;
    for (const auto& link : items)
    {
        if (m_itemService->item(*link.itemId, userId).has_value() && m_documentService->checkDocumentAccess(*link.documentId, userId, false).has_value())
        {
            filtered.push_back(link);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::ItemDocument> ItemDocumentService::getItemDocument(
    int64_t id,
    int64_t userId
)
{
    return checkItemDocumentAccess(id, userId, false);
}

std::vector<dto::ItemDocument> ItemDocumentService::getDocumentsByItem(
    int64_t itemId,
    int64_t userId
)
{
    if (!m_itemService->item(itemId, userId).has_value())
    {
        LOG_WARN
            << "getDocumentsByItem: пользователь " << userId
            << " не имеет доступа к элементу " << itemId;
        return {};
    }

    auto links = m_itemDocumentRepo->findByItemId(itemId);

    // Фильтруем по доступу к документам
    std::vector<dto::ItemDocument> filtered;
    for (const auto& link : links)
    {
        if (m_documentService->checkDocumentAccess(*link.documentId, userId, false).has_value())
        {
            filtered.push_back(link);
        }
    }

    return filtered;
}

std::vector<dto::ItemDocument> ItemDocumentService::getItemsByDocument(
    int64_t documentId,
    int64_t userId
)
{
    auto document = m_documentService->checkDocumentAccess(documentId, userId, false);
    if (!document.has_value())
    {
        LOG_WARN
            << "getItemsByDocument: пользователь " << userId
            << " не имеет доступа к документу " << documentId;
        return {};
    }

    auto links = m_itemDocumentRepo->findByDocumentId(documentId);

    // Фильтруем по доступу к элементам
    std::vector<dto::ItemDocument> filtered;
    for (const auto& link : links)
    {
        if (m_itemService->item(*link.itemId, userId).has_value())
        {
            filtered.push_back(link);
        }
    }

    return filtered;
}

std::optional<dto::ItemDocument> ItemDocumentService::createItemDocument(
    const dto::ItemDocument& itemDocument,
    int64_t userId
)
{
    if (!itemDocument.itemId.has_value() || !itemDocument.documentId.has_value())
    {
        LOG_WARN << "createItemDocument: отсутствуют обязательные поля";
        return std::nullopt;
    }

    if (!canLinkDocumentToItem(*itemDocument.itemId, *itemDocument.documentId, userId))
    {
        LOG_WARN
            << "createItemDocument: недостаточно прав для привязки документа "
            << *itemDocument.documentId << " к элементу " << *itemDocument.itemId;
        return std::nullopt;
    }

    // Проверяем, не существует ли уже такой связи
    if (m_itemDocumentRepo->exists(*itemDocument.itemId, *itemDocument.documentId))
    {
        LOG_WARN
            << "createItemDocument: связь уже существует, itemId="
            << *itemDocument.itemId << ", documentId=" << *itemDocument.documentId;
        return std::nullopt;
    }

    const int64_t newId = m_itemDocumentRepo->create(itemDocument);
    if (newId <= 0)
    {
        LOG_ERROR << "createItemDocument: не удалось создать связь";
        return std::nullopt;
    }

    LOG_INFO
        << "Связь элемента с документом создана: id=" << newId
        << ", itemId=" << *itemDocument.itemId
        << ", documentId=" << *itemDocument.documentId
        << ", пользователь=" << userId;

    return m_itemDocumentRepo->findById(newId);
}

ItemDocumentResult ItemDocumentService::deleteItemDocument(
    int64_t id,
    int64_t userId
)
{
    ItemDocumentResult result;

    auto link = checkItemDocumentAccess(id, userId, true);
    if (!link.has_value())
    {
        result.errorMessage = "Связь не найдена или нет доступа";
        result.errorCode = 404;
        return result;
    }

    if (!m_itemDocumentRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить связь";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Связь элемента с документом удалена: id=" << id
        << ", пользователь=" << userId;

    return result;
}

int64_t ItemDocumentService::deleteItemDocumentsByItem(
    int64_t itemId,
    int64_t userId
)
{
    if (!m_itemService->item(itemId, userId).has_value())
    {
        LOG_WARN
            << "deleteItemDocumentsByItem: пользователь " << userId
            << " не имеет доступа к элементу " << itemId;
        return 0;
    }

    // Проверяем право на запись к элементу
    // Для удаления связей нужно право на запись
    auto item = m_itemService->item(itemId, userId);
    if (!item.has_value())
    {
        return 0;
    }

    return m_itemDocumentRepo->removeByItemId(itemId);
}

int64_t ItemDocumentService::deleteItemDocumentsByDocument(
    int64_t documentId,
    int64_t userId
)
{
    auto document = m_documentService->checkDocumentAccess(documentId, userId, true);
    if (!document.has_value())
    {
        LOG_WARN
            << "deleteItemDocumentsByDocument: пользователь " << userId
            << " не имеет доступа к документу " << documentId;
        return 0;
    }

    return m_itemDocumentRepo->removeByDocumentId(documentId);
}

std::optional<dto::ItemDocument> ItemDocumentService::checkItemDocumentAccess(
    int64_t id,
    int64_t userId,
    bool needWrite
)
{
    auto link = m_itemDocumentRepo->findById(id);
    if (!link.has_value())
    {
        LOG_DEBUG << "checkItemDocumentAccess: связь не найдена, id=" << id;
        return std::nullopt;
    }

    if (!m_itemService->item(*link->itemId, userId).has_value())
    {
        LOG_WARN
            << "checkItemDocumentAccess: нет доступа к элементу "
            << *link->itemId;
        return std::nullopt;
    }

    if (!m_documentService->checkDocumentAccess(*link->documentId, userId, needWrite).has_value())
    {
        LOG_WARN
            << "checkItemDocumentAccess: нет доступа к документу "
            << *link->documentId;
        return std::nullopt;
    }

    return link;
}

bool ItemDocumentService::canLinkDocumentToItem(
    int64_t itemId,
    int64_t documentId,
    int64_t userId
)
{
    // Проверяем доступ к элементу (нужно право на запись)
    auto item = m_itemService->item(itemId, userId);
    if (!item.has_value())
    {
        LOG_DEBUG
            << "canLinkDocumentToItem: нет доступа к элементу " << itemId;
        return false;
    }

    // Проверяем доступ к документу (нужно право на запись)
    auto document = m_documentService->checkDocumentAccess(documentId, userId, true);
    if (!document.has_value())
    {
        LOG_DEBUG
            << "canLinkDocumentToItem: нет доступа к документу " << documentId;
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
