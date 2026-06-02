#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iitem_type_service.h"
#include "repo/item_type_repository.h"

namespace server
{
namespace services
{

class ItemTypeService final : public IItemTypeService
{
public:
    explicit ItemTypeService(
        std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    ItemTypesPage itemTypes(
        int page,
        int pageSize,
        std::optional<int64_t> workflowId = std::nullopt,
        std::optional<std::string> kind = std::nullopt,
        const std::string& searchCaption = ""
    ) override;

    std::optional<dto::ItemType> itemType(int64_t id) override;

    std::optional<dto::ItemType> createItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) override;

    std::optional<dto::ItemType> updateItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) override;

    bool deleteItemType(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::ItemType> itemTypesByWorkflow(int64_t workflowId) override;

private:
    std::shared_ptr<repositories::IItemTypeRepository> m_itemTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
