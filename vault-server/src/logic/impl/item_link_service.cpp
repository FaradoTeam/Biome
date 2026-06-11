#include "common/log/log.h"

#include "item_link_service.h"

namespace server::services
{

ItemLinkService::ItemLinkService(
    std::shared_ptr<repositories::IItemLinkRepository> itemLinkRepo,
    std::shared_ptr<repositories::ILinkTypeRepository> linkTypeRepo,
    std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
    std::shared_ptr<IItemService> itemService,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_itemLinkRepo(std::move(itemLinkRepo))
    , m_linkTypeRepo(std::move(linkTypeRepo))
    , m_phaseRepo(std::move(phaseRepo))
    , m_itemService(std::move(itemService))
    , m_authzService(std::move(authzService))
{
    if (!m_itemLinkRepo
        || !m_linkTypeRepo
        || !m_phaseRepo
        || !m_itemService
        || !m_authzService)
    {
        throw std::runtime_error(
            "ItemLinkService: один или несколько компонентов не инициализированы"
        );
    }
}

ItemLinksPage ItemLinkService::getItemLinks(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> linkTypeId,
    std::optional<int64_t> sourceItemId,
    std::optional<int64_t> destinationItemId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Проверяем доступ к элементам-фильтрам
    if (sourceItemId.has_value() && !checkItemAccess(*sourceItemId, userId, false))
    {
        LOG_WARN << "getItemLinks: пользователь " << userId
                 << " не имеет доступа на чтение к элементу " << *sourceItemId;
        return { {}, 0 };
    }
    if (destinationItemId.has_value() && !checkItemAccess(*destinationItemId, userId, false))
    {
        LOG_WARN << "getItemLinks: пользователь " << userId
                 << " не имеет доступа на чтение к элементу " << *destinationItemId;
        return { {}, 0 };
    }

    auto [links, total] = m_itemLinkRepo->findAll(
        page, pageSize, linkTypeId, sourceItemId, destinationItemId
    );

    // Фильтруем по правам на чтение для обоих элементов каждой связи
    std::vector<dto::ItemLink> filtered;
    for (const auto& link : links)
    {
        if (checkItemAccess(*link.sourceItemId, userId, false) && checkItemAccess(*link.destinationItemId, userId, false))
        {
            filtered.push_back(link);
        }
    }

    return { filtered, static_cast<int64_t>(filtered.size()) };
}

std::optional<dto::ItemLink> ItemLinkService::getItemLink(
    int64_t id,
    int64_t userId
)
{
    return checkLinkAccess(id, userId, false);
}

std::vector<dto::ItemLink> ItemLinkService::getItemLinksByItemId(
    int64_t itemId,
    int64_t userId
)
{
    if (!checkItemAccess(itemId, userId, false))
    {
        LOG_WARN
            << "getItemLinksByItemId: пользователь " << userId
            << " не имеет доступа на чтение к элементу " << itemId;
        return {};
    }

    auto links = m_itemLinkRepo->findByItemId(itemId);

    std::vector<dto::ItemLink> filtered;
    for (const auto& link : links)
    {
        const int64_t otherId = (link.sourceItemId == itemId)
            ? *link.destinationItemId
            : *link.sourceItemId;
        if (checkItemAccess(otherId, userId, false))
        {
            filtered.push_back(link);
        }
    }
    return filtered;
}

std::vector<dto::ItemLink> ItemLinkService::getItemLinksByLinkTypeId(
    int64_t linkTypeId,
    int64_t userId
)
{
    auto links = m_itemLinkRepo->findByLinkTypeId(linkTypeId);

    std::vector<dto::ItemLink> filtered;
    for (const auto& link : links)
    {
        if (checkItemAccess(*link.sourceItemId, userId, false)
            && checkItemAccess(*link.destinationItemId, userId, false))
        {
            filtered.push_back(link);
        }
    }
    return filtered;
}

std::optional<dto::ItemLink> ItemLinkService::createItemLink(
    const dto::ItemLink& itemLink,
    int64_t userId
)
{
    std::string errorMessage;
    if (!validateItemLink(itemLink, errorMessage))
    {
        LOG_WARN << "createItemLink: " << errorMessage;
        return std::nullopt;
    }

    int64_t linkTypeId = *itemLink.linkTypeId;
    int64_t sourceId = *itemLink.sourceItemId;
    int64_t destId = *itemLink.destinationItemId;

    // Проверяем существование типа связи
    auto linkType = m_linkTypeRepo->findById(linkTypeId);
    if (!linkType)
    {
        LOG_WARN << "createItemLink: тип связи не найден, id=" << linkTypeId;
        return std::nullopt;
    }

    // Проверяем права на запись для обоих элементов
    if (!checkItemAccess(sourceId, userId, true))
    {
        LOG_WARN << "createItemLink: недостаточно прав на запись для элемента " << sourceId;
        return std::nullopt;
    }
    if (!checkItemAccess(destId, userId, true))
    {
        LOG_WARN << "createItemLink: недостаточно прав на запись для элемента " << destId;
        return std::nullopt;
    }

    // Проверяем уникальность тройки
    if (m_itemLinkRepo->existsByTriple(linkTypeId, sourceId, destId))
    {
        LOG_WARN << "createItemLink: такая связь уже существует";
        return std::nullopt;
    }

    const int64_t newId = m_itemLinkRepo->create(itemLink);
    if (newId <= 0)
    {
        LOG_ERROR << "createItemLink: не удалось создать связь";
        return std::nullopt;
    }

    LOG_INFO
        << "Связь элементов создана: id=" << newId
        << ", тип=" << linkTypeId
        << ", source=" << sourceId << ", dest=" << destId
        << ", пользователь=" << userId;

    return m_itemLinkRepo->findById(newId);
}

ItemLinkResult ItemLinkService::deleteItemLink(
    int64_t id,
    int64_t userId
)
{
    ItemLinkResult result;

    auto link = checkLinkAccess(id, userId, true);
    if (!link)
    {
        result.errorCode = 404;
        result.errorMessage = "Связь не найдена или нет доступа";
        return result;
    }

    if (!m_itemLinkRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить связь";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO << "Связь элементов удалена: id=" << id << ", пользователь=" << userId;
    return result;
}

// ============================================================
// Приватные методы
// ============================================================

bool ItemLinkService::checkItemAccess(int64_t itemId, int64_t userId, bool needWrite)
{
    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return true;
    }

    // Получаем элемент через IItemService (проверяет права на чтение)
    auto item = m_itemService->item(itemId, userId);
    if (!item)
    {
        LOG_DEBUG << "checkItemAccess: элемент " << itemId << " не найден или нет доступа";
        return false;
    }

    if (needWrite)
    {
        return checkItemWriteAccess(itemId, userId);
    }
    return true;
}

bool ItemLinkService::checkItemWriteAccess(int64_t itemId, int64_t userId)
{
    auto projectId = getProjectIdByItemId(itemId);
    if (!projectId)
    {
        LOG_WARN << "checkItemWriteAccess: не удалось определить проект для элемента " << itemId;
        return false;
    }

    auto authz = m_authzService->canWriteToProject(userId, *projectId);
    if (!authz.granted)
    {
        LOG_DEBUG << "checkItemWriteAccess: пользователь " << userId
                  << " не имеет права на запись в проекте " << *projectId;
        return false;
    }
    return true;
}

std::optional<dto::ItemLink> ItemLinkService::checkLinkAccess(
    int64_t id,
    int64_t userId,
    bool needWrite
)
{
    auto link = m_itemLinkRepo->findById(id);
    if (!link)
    {
        LOG_DEBUG << "checkLinkAccess: связь не найдена, id=" << id;
        return std::nullopt;
    }

    if (!checkItemAccess(*link->sourceItemId, userId, needWrite)
        || !checkItemAccess(*link->destinationItemId, userId, needWrite))
    {
        LOG_WARN << "checkLinkAccess: недостаточно прав для доступа к связи " << id;
        return std::nullopt;
    }
    return link;
}

bool ItemLinkService::validateItemLink(const dto::ItemLink& link, std::string& errorMessage)
{
    if (!link.linkTypeId.has_value())
    {
        errorMessage = "linkTypeId является обязательным полем";
        return false;
    }
    if (!link.sourceItemId.has_value())
    {
        errorMessage = "sourceItemId является обязательным полем";
        return false;
    }
    if (!link.destinationItemId.has_value())
    {
        errorMessage = "destinationItemId является обязательным полем";
        return false;
    }
    if (*link.sourceItemId == *link.destinationItemId)
    {
        errorMessage = "элемент не может быть связан сам с собой";
        return false;
    }
    return true;
}

std::optional<int64_t> ItemLinkService::getProjectIdByItemId(int64_t itemId)
{
    auto item = m_itemService->item(itemId, 0); // userId=0, но itemService уже проверяет права, здесь нам нужен только item
    if (!item || !item->phaseId.has_value())
    {
        LOG_WARN << "getProjectIdByItemId: элемент " << itemId << " не найден или не имеет phaseId";
        return std::nullopt;
    }

    auto phase = m_phaseRepo->findById(*item->phaseId);
    if (!phase || !phase->projectId.has_value())
    {
        LOG_WARN << "getProjectIdByItemId: фаза " << *item->phaseId << " не найдена или не имеет projectId";
        return std::nullopt;
    }
    return *phase->projectId;
}

} // namespace server::services
