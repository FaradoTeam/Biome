#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/ilink_type_service.h"
#include "repo/link_type_repository.h"

namespace server::services
{

class LinkTypeService final : public ILinkTypeService
{
public:
    LinkTypeService(
        std::shared_ptr<repositories::ILinkTypeRepository> linkTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    LinkTypesPage getLinkTypes(
        int page,
        int pageSize,
        std::optional<int64_t> sourceItemTypeId = std::nullopt,
        std::optional<int64_t> destinationItemTypeId = std::nullopt
    ) override;

    std::optional<dto::LinkType> getLinkType(int64_t id) override;

    std::optional<dto::LinkType> createLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) override;

    std::optional<dto::LinkType> updateLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) override;

    bool deleteLinkType(
        int64_t id,
        int64_t userId
    ) override;

private:
    bool validateLinkType(const dto::LinkType& linkType, std::string& errorMessage);

private:
    std::shared_ptr<repositories::ILinkTypeRepository> m_linkTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace server::services
