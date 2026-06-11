#include "common/log/log.h"

#include "link_type_service.h"

namespace server::services
{

LinkTypeService::LinkTypeService(
    std::shared_ptr<repositories::ILinkTypeRepository> linkTypeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_linkTypeRepo(std::move(linkTypeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_linkTypeRepo)
    {
        throw std::runtime_error("LinkTypeService: репозиторий не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("LinkTypeService: сервис авторизации не инициализирован");
    }
}

LinkTypesPage LinkTypeService::getLinkTypes(
    int page,
    int pageSize,
    std::optional<int64_t> sourceItemTypeId,
    std::optional<int64_t> destinationItemTypeId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [linkTypes, total] = m_linkTypeRepo->findAll(
        page, pageSize, sourceItemTypeId, destinationItemTypeId
    );
    return { linkTypes, total };
}

std::optional<dto::LinkType> LinkTypeService::getLinkType(int64_t id)
{
    return m_linkTypeRepo->findById(id);
}

std::optional<dto::LinkType> LinkTypeService::createLinkType(
    const dto::LinkType& linkType,
    int64_t userId
)
{
    // Только супер-админ может создавать типы связей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createLinkType: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    std::string errorMessage;
    if (!validateLinkType(linkType, errorMessage))
    {
        LOG_WARN << "createLinkType: " << errorMessage;
        return std::nullopt;
    }

    const int64_t newId = m_linkTypeRepo->create(linkType);
    if (newId <= 0)
    {
        LOG_ERROR << "createLinkType: не удалось создать тип связи";
        return std::nullopt;
    }

    LOG_INFO << "Тип связи создан: id=" << newId << ", пользователь=" << userId;
    return m_linkTypeRepo->findById(newId);
}

std::optional<dto::LinkType> LinkTypeService::updateLinkType(
    const dto::LinkType& linkType,
    int64_t userId
)
{
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateLinkType: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!linkType.id.has_value())
    {
        LOG_WARN << "updateLinkType: отсутствует ID";
        return std::nullopt;
    }

    auto existing = m_linkTypeRepo->findById(*linkType.id);
    if (!existing)
    {
        LOG_WARN << "updateLinkType: тип связи не найден, id=" << *linkType.id;
        return std::nullopt;
    }

    if (!m_linkTypeRepo->update(linkType))
    {
        LOG_ERROR << "updateLinkType: не удалось обновить тип связи id=" << *linkType.id;
        return std::nullopt;
    }

    LOG_INFO << "Тип связи обновлён: id=" << *linkType.id << ", пользователь=" << userId;
    return m_linkTypeRepo->findById(*linkType.id);
}

bool LinkTypeService::deleteLinkType(int64_t id, int64_t userId)
{
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteLinkType: пользователь " << userId << " не имеет прав";
        return false;
    }

    if (!m_linkTypeRepo->exists(id))
    {
        LOG_WARN << "deleteLinkType: тип связи не найден, id=" << id;
        return false;
    }

    if (m_linkTypeRepo->isUsed(id))
    {
        LOG_WARN << "deleteLinkType: тип связи используется, id=" << id;
        return false;
    }

    if (!m_linkTypeRepo->remove(id))
    {
        LOG_ERROR << "deleteLinkType: не удалось удалить тип связи id=" << id;
        return false;
    }

    LOG_INFO << "Тип связи удалён: id=" << id << ", пользователь=" << userId;
    return true;
}

bool LinkTypeService::validateLinkType(
    const dto::LinkType& linkType,
    std::string& errorMessage
)
{
    if (!linkType.sourceItemTypeId.has_value())
    {
        errorMessage = "sourceItemTypeId является обязательным полем";
        return false;
    }
    if (!linkType.destinationItemTypeId.has_value())
    {
        errorMessage = "destinationItemTypeId является обязательным полем";
        return false;
    }
    if (!linkType.caption.has_value() || linkType.caption->empty())
    {
        errorMessage = "caption является обязательным полем";
        return false;
    }
    if (linkType.caption->length() > 255)
    {
        errorMessage = "caption не может превышать 255 символов";
        return false;
    }
    return true;
}

} // namespace server::services
