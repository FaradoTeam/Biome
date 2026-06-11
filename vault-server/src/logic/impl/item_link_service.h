#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iitem_link_service.h"
#include "logic/iitem_service.h"

#include "repo/item_link_repository.h"
#include "repo/link_type_repository.h"
#include "repo/phase_repository.h"

namespace server::services
{

class ItemLinkService final : public IItemLinkService
{
public:
    ItemLinkService(
        std::shared_ptr<repositories::IItemLinkRepository> itemLinkRepo,
        std::shared_ptr<repositories::ILinkTypeRepository> linkTypeRepo,
        std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
        std::shared_ptr<IItemService> itemService,
        std::shared_ptr<IAuthorizationService> authzService
    );

    ItemLinksPage getItemLinks(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> linkTypeId = std::nullopt,
        std::optional<int64_t> sourceItemId = std::nullopt,
        std::optional<int64_t> destinationItemId = std::nullopt
    ) override;

    std::optional<dto::ItemLink> getItemLink(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::ItemLink> getItemLinksByItemId(
        int64_t itemId,
        int64_t userId
    ) override;

    std::vector<dto::ItemLink> getItemLinksByLinkTypeId(
        int64_t linkTypeId,
        int64_t userId
    ) override;

    std::optional<dto::ItemLink> createItemLink(
        const dto::ItemLink& itemLink,
        int64_t userId
    ) override;

    ItemLinkResult deleteItemLink(
        int64_t id,
        int64_t userId
    ) override;

private:
    bool checkItemAccess(int64_t itemId, int64_t userId, bool needWrite = false);
    bool checkItemWriteAccess(int64_t itemId, int64_t userId);
    std::optional<dto::ItemLink> checkLinkAccess(int64_t id, int64_t userId, bool needWrite = false);
    bool validateItemLink(const dto::ItemLink& link, std::string& errorMessage);
    std::optional<int64_t> getProjectIdByItemId(int64_t itemId);

private:
    std::shared_ptr<repositories::IItemLinkRepository> m_itemLinkRepo;
    std::shared_ptr<repositories::ILinkTypeRepository> m_linkTypeRepo;
    std::shared_ptr<repositories::IPhaseRepository> m_phaseRepo;
    std::shared_ptr<IItemService> m_itemService;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
