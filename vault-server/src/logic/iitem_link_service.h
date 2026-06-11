#pragma once

#include <optional>
#include <vector>

#include "common/dto/item_link.h"

namespace server::services
{

struct ItemLinksPage
{
    std::vector<dto::ItemLink> links;
    int64_t totalCount = 0;
};

struct ItemLinkResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления связями между элементами.
 */
class IItemLinkService
{
public:
    virtual ~IItemLinkService() = default;

    virtual ItemLinksPage getItemLinks(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> linkTypeId = std::nullopt,
        std::optional<int64_t> sourceItemId = std::nullopt,
        std::optional<int64_t> destinationItemId = std::nullopt
    ) = 0;

    virtual std::optional<dto::ItemLink> getItemLink(
        int64_t id,
        int64_t userId
    ) = 0;

    virtual std::vector<dto::ItemLink> getItemLinksByItemId(
        int64_t itemId,
        int64_t userId
    ) = 0;

    virtual std::vector<dto::ItemLink> getItemLinksByLinkTypeId(
        int64_t linkTypeId,
        int64_t userId
    ) = 0;

    virtual std::optional<dto::ItemLink> createItemLink(
        const dto::ItemLink& itemLink,
        int64_t userId
    ) = 0;

    virtual ItemLinkResult deleteItemLink(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
