#pragma once

#include <optional>
#include <vector>

#include "common/dto/link_type.h"

namespace server::services
{

struct LinkTypesPage
{
    std::vector<dto::LinkType> linkTypes;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления типами связей.
 */
class ILinkTypeService
{
public:
    virtual ~ILinkTypeService() = default;

    virtual LinkTypesPage getLinkTypes(
        int page,
        int pageSize,
        std::optional<int64_t> sourceItemTypeId = std::nullopt,
        std::optional<int64_t> destinationItemTypeId = std::nullopt
    ) = 0;

    virtual std::optional<dto::LinkType> getLinkType(int64_t id) = 0;

    virtual std::optional<dto::LinkType> createLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) = 0;

    virtual std::optional<dto::LinkType> updateLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) = 0;

    virtual bool deleteLinkType(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace server::services
