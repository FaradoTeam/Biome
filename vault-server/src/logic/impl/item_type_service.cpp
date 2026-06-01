#include "common/log/log.h"

#include "item_type_service.h"

namespace server
{
namespace services
{

ItemTypeService::ItemTypeService(
    std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo
)
    : m_itemTypeRepo(std::move(itemTypeRepo))
{
    if (!m_itemTypeRepo)
    {
        throw std::runtime_error("ItemTypeRepository не может быть пустым");
    }
}

ItemTypesPage ItemTypeService::itemTypes(
    int page,
    int pageSize,
    std::optional<int64_t> workflowId,
    std::optional<std::string> kind,
    const std::string& searchCaption
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [itemTypeList, total] = m_itemTypeRepo->findAll(
        page,
        pageSize,
        workflowId,
        kind,
        searchCaption
    );
    return { itemTypeList, total };
}

std::optional<dto::ItemType> ItemTypeService::itemType(int64_t id)
{
    return m_itemTypeRepo->findById(id);
}

std::optional<dto::ItemType> ItemTypeService::createItemType(
    const dto::ItemType& itemType
)
{
    // Валидация на уровне бизнес-логики
    if (!itemType.caption.has_value() || itemType.caption->empty())
    {
        LOG_WARN
            << "createItemType: название типа элемента не может быть пустым";
        return std::nullopt;
    }

    if (!itemType.workflowId.has_value())
    {
        LOG_WARN << "createItemType: не указан workflowId";
        return std::nullopt;
    }

    if (!itemType.kind.has_value())
    {
        LOG_WARN << "createItemType: не указан kind";
        return std::nullopt;
    }

    const int64_t newId = m_itemTypeRepo->create(itemType);
    if (newId <= 0)
    {
        LOG_ERROR
            << "createItemType: ошибка при создании типа элемента в репозитории";
        return std::nullopt;
    }

    LOG_INFO << "Создан новый тип элемента с id=" << newId;
    return m_itemTypeRepo->findById(newId);
}

std::optional<dto::ItemType> ItemTypeService::updateItemType(
    const dto::ItemType& itemType
)
{
    if (!itemType.id.has_value())
    {
        LOG_WARN << "updateItemType: отсутствует ID типа элемента";
        return std::nullopt;
    }

    if (!m_itemTypeRepo->exists(itemType.id.value()))
    {
        LOG_WARN
            << "updateItemType: тип элемента с id=" << itemType.id.value()
            << " не найден";
        return std::nullopt;
    }

    if (!m_itemTypeRepo->update(itemType))
    {
        LOG_ERROR
            << "updateItemType: ошибка при обновлении типа элемента в репозитории";
        return std::nullopt;
    }

    LOG_INFO
        << "Тип элемента с id=" << itemType.id.value()
        << " успешно обновлен";
    return m_itemTypeRepo->findById(itemType.id.value());
}

bool ItemTypeService::deleteItemType(int64_t id)
{
    if (!m_itemTypeRepo->exists(id))
    {
        LOG_WARN << "deleteItemType: тип элемента с id=" << id << " не найден";
        return false;
    }

    if (m_itemTypeRepo->remove(id))
    {
        LOG_INFO << "Тип элемента с id=" << id << " успешно удален";
        return true;
    }

    LOG_ERROR << "deleteItemType: ошибка при удалении типа элемента с id=" << id;
    return false;
}

std::vector<dto::ItemType> ItemTypeService::itemTypesByWorkflow(int64_t workflowId)
{
    return m_itemTypeRepo->findByWorkflow(workflowId);
}

} // namespace services
} // namespace server
