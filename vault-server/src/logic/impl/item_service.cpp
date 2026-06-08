#include "common/log/log.h"

#include "item_service.h"

namespace server
{
namespace services
{

ItemService::ItemService(
    std::shared_ptr<repositories::IItemRepository> itemRepo,
    std::shared_ptr<repositories::IItemFieldRepository> itemFieldRepo,
    std::shared_ptr<repositories::IItemTypeRepository> itemTypeRepo,
    std::shared_ptr<repositories::IPhaseRepository> phaseRepo,
    std::shared_ptr<repositories::IProjectRepository> projectRepo,
    std::shared_ptr<repositories::IStateRepository> stateRepo,
    std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_itemRepo(std::move(itemRepo))
    , m_itemFieldRepo(std::move(itemFieldRepo))
    , m_itemTypeRepo(std::move(itemTypeRepo))
    , m_phaseRepo(std::move(phaseRepo))
    , m_projectRepo(std::move(projectRepo))
    , m_stateRepo(std::move(stateRepo))
    , m_fieldTypeRepo(std::move(fieldTypeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_itemRepo
        || !m_itemFieldRepo
        || !m_itemTypeRepo
        || !m_phaseRepo
        || !m_projectRepo
        || !m_stateRepo
        || !m_fieldTypeRepo)
    {
        throw std::runtime_error(
            "ItemService: один или несколько репозиториев не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error("ItemService: сервис авторизации не инициализирован");
    }
}

ItemsPage ItemService::items(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> itemTypeId,
    std::optional<int64_t> parentId,
    std::optional<int64_t> phaseId,
    std::optional<int64_t> stateId,
    std::optional<bool> isDeleted,
    const std::string& searchCaption
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    // Получаем ID проектов, доступных пользователю
    const std::vector<int64_t> accessibleProjectIds = getAccessibleProjectIds(userId);

    auto [items, total] = m_itemRepo->findAll(
        page, pageSize, itemTypeId, parentId, phaseId, stateId,
        isDeleted, searchCaption, accessibleProjectIds
    );

    return { items, total };
}

std::optional<dto::Item> ItemService::item(int64_t id, int64_t userId)
{
    auto item = checkItemAccess(id, userId, false);
    if (!item.has_value())
    {
        return std::nullopt;
    }
    return item;
}

std::optional<dto::Item> ItemService::createItem(
    const dto::Item& item,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateItem(item, errorMessage))
    {
        LOG_WARN << "createItem: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем существование фазы и получаем ID проекта
    auto phase = m_phaseRepo->findById(*item.phaseId);
    if (!phase.has_value() || !phase->projectId.has_value())
    {
        LOG_WARN << "createItem: фаза не найдена или не имеет projectId";
        return std::nullopt;
    }
    const int64_t projectId = *phase->projectId;

    // 3. Проверяем существование типа элемента
    auto itemType = m_itemTypeRepo->findById(*item.itemTypeId);
    if (!itemType.has_value())
    {
        LOG_WARN << "createItem: тип элемента не найден, id=" << *item.itemTypeId;
        return std::nullopt;
    }

    // 4. Проверяем существование состояния
    auto state = m_stateRepo->findById(*item.stateId);
    if (!state.has_value())
    {
        LOG_WARN << "createItem: состояние не найдено, id=" << *item.stateId;
        return std::nullopt;
    }

    // 5. Проверяем права на создание элемента
    //    - Право на запись в проекте
    auto writeProjectAuthz = m_authzService->canWriteToProject(userId, projectId);
    if (!writeProjectAuthz.granted)
    {
        LOG_WARN
            << "createItem: пользователь " << userId
            << " не имеет права на запись в проекте " << projectId;
        return std::nullopt;
    }

    //    - Право на запись типа элемента
    auto writeItemTypeAuthz = m_authzService->canWriteItemType(
        userId, projectId, *item.itemTypeId
    );
    if (!writeItemTypeAuthz.granted)
    {
        LOG_WARN
            << "createItem: пользователь " << userId
            << " не имеет права на создание элементов типа "
            << *item.itemTypeId;
        return std::nullopt;
    }

    // 6. Если указан родительский элемент, проверяем его существование и права
    if (item.parentId.has_value())
    {
        auto parentItem = checkItemAccess(*item.parentId, userId, false);
        if (!parentItem.has_value())
        {
            LOG_WARN << "createItem: родительский элемент не найден или нет доступа";
            return std::nullopt;
        }
        // Родитель должен быть в той же фазе
        if (parentItem->phaseId != item.phaseId)
        {
            LOG_WARN << "createItem: родительский элемент в другой фазе";
            return std::nullopt;
        }
    }

    // 7. Создаём элемент
    const int64_t newId = m_itemRepo->create(item);
    if (newId <= 0)
    {
        LOG_ERROR << "createItem: не удалось создать элемент";
        return std::nullopt;
    }

    LOG_INFO
        << "Элемент создан: id=" << newId
        << ", пользователь=" << userId;

    return m_itemRepo->findById(newId);
}

std::optional<dto::Item> ItemService::updateItem(
    const dto::Item& item,
    int64_t userId
)
{
    if (!item.id.has_value())
    {
        LOG_WARN << "updateItem: отсутствует ID элемента";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к элементу
    auto existing = checkItemAccess(*item.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Если меняется фаза, проверяем права на новую фазу
    std::optional<int64_t> newProjectId;
    if (item.phaseId.has_value() && *item.phaseId != *existing->phaseId)
    {
        auto newPhase = m_phaseRepo->findById(*item.phaseId);
        if (!newPhase.has_value() || !newPhase->projectId.has_value())
        {
            LOG_WARN << "updateItem: новая фаза не найдена";
            return std::nullopt;
        }
        newProjectId = *newPhase->projectId;

        // Проверяем права на запись в новом проекте
        auto writeAuthz = m_authzService->canWriteToProject(userId, *newProjectId);
        if (!writeAuthz.granted)
        {
            LOG_WARN << "updateItem: пользователь " << userId
                     << " не имеет права на запись в проекте " << *newProjectId;
            return std::nullopt;
        }
    }

    // 3. Если меняется тип элемента, проверяем права на новый тип
    if (item.itemTypeId.has_value() && *item.itemTypeId != *existing->itemTypeId)
    {
        // Получаем ID проекта (из существующей фазы или новой)
        const int64_t projectId = newProjectId.has_value()
            ? *newProjectId
            : *getProjectIdByPhaseId(*existing->phaseId);

        auto writeItemTypeAuthz = m_authzService->canWriteItemType(
            userId, projectId, *item.itemTypeId
        );
        if (!writeItemTypeAuthz.granted)
        {
            LOG_WARN
                << "updateItem: пользователь " << userId
                << " не имеет права на изменение элементов типа "
                << *item.itemTypeId;
            return std::nullopt;
        }
    }

    // 4. Если меняется состояние, проверяем право на переход
    if (item.stateId.has_value() && *item.stateId != *existing->stateId)
    {
        const int64_t projectId = newProjectId.has_value()
            ? *newProjectId
            : *getProjectIdByPhaseId(*existing->phaseId);

        auto stateAuthz = m_authzService->canTransitionToState(
            userId, projectId, *item.stateId
        );
        if (!stateAuthz.granted)
        {
            LOG_WARN
                << "updateItem: пользователь " << userId
                << " не имеет права на переход в состояние "
                << *item.stateId;
            return std::nullopt;
        }
    }

    // 5. Обновляем элемент
    if (!m_itemRepo->update(item))
    {
        LOG_ERROR << "updateItem: не удалось обновить элемент id=" << *item.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Элемент обновлён: id=" << *item.id
        << ", пользователь=" << userId;

    return m_itemRepo->findById(*item.id);
}

ItemResult ItemService::deleteItem(int64_t id, int64_t userId)
{
    ItemResult result;

    // 1. Проверяем существование и доступ к элементу
    auto existing = checkItemAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Элемент не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Если элемент уже удалён
    if (existing->isDeleted.value_or(false))
    {
        result.errorMessage = "Элемент уже удалён";
        result.errorCode = 400;
        return result;
    }

    // 3. Мягкое удаление
    if (!m_itemRepo->softDelete(id))
    {
        result.errorMessage = "Не удалось удалить элемент";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO << "Элемент удалён: id=" << id << ", пользователь=" << userId;

    return result;
}

ItemResult ItemService::restoreItem(int64_t id, int64_t userId)
{
    ItemResult result;

    // 1. Проверяем существование и доступ к элементу
    auto existing = checkItemAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Элемент не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Если элемент не удалён
    if (!existing->isDeleted.value_or(false))
    {
        result.errorMessage = "Элемент не был удалён";
        result.errorCode = 400;
        return result;
    }

    // 3. Восстанавливаем
    if (!m_itemRepo->restore(id))
    {
        result.errorMessage = "Не удалось восстановить элемент";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO << "Элемент восстановлен: id=" << id << ", пользователь=" << userId;

    return result;
}

std::vector<dto::ItemField> ItemService::getItemFields(
    int64_t itemId,
    int64_t userId
)
{
    // Проверяем доступ к элементу
    auto item = checkItemAccess(itemId, userId, false);
    if (!item.has_value())
    {
        return {};
    }

    return m_itemFieldRepo->findByItemId(itemId);
}

std::optional<dto::ItemField> ItemService::getItemField(
    int64_t itemId,
    int64_t fieldTypeId,
    int64_t userId
)
{
    // Проверяем доступ к элементу
    auto item = checkItemAccess(itemId, userId, false);
    if (!item.has_value())
    {
        return std::nullopt;
    }

    return m_itemFieldRepo->findByItemAndFieldType(itemId, fieldTypeId);
}

std::optional<dto::ItemField> ItemService::setItemField(
    const dto::ItemField& field,
    int64_t userId
)
{
    // 1. Валидация
    if (!field.itemId.has_value() || !field.fieldTypeId.has_value())
    {
        LOG_WARN << "setItemField: отсутствуют обязательные поля";
        return std::nullopt;
    }

    // 2. Проверяем доступ к элементу и права на запись
    auto item = checkItemAccess(*field.itemId, userId, true);
    if (!item.has_value())
    {
        LOG_WARN << "setItemField: элемент не найден или нет доступа";
        return std::nullopt;
    }

    // 3. Получаем ID проекта
    auto projectId = getProjectIdByPhaseId(*item->phaseId);
    if (!projectId.has_value())
    {
        LOG_WARN << "setItemField: не удалось определить проект";
        return std::nullopt;
    }

    // 4. Проверяем доступ к типу поля
    if (!checkFieldTypeAccess(*field.fieldTypeId, userId, *projectId, true))
    {
        LOG_WARN
            << "setItemField: нет доступа к типу поля " << *field.fieldTypeId;
        return std::nullopt;
    }

    // 5. Получаем тип поля для валидации значения
    auto fieldType = m_fieldTypeRepo->findById(*field.fieldTypeId);
    if (!fieldType.has_value())
    {
        LOG_WARN
            << "setItemField: тип поля не найден, id=" << *field.fieldTypeId;
        return std::nullopt;
    }

    // 6. Валидируем значение
    std::string errorMessage;
    if (!validateItemField(field, *fieldType, errorMessage))
    {
        LOG_WARN << "setItemField: " << errorMessage;
        return std::nullopt;
    }

    // 7. Проверяем, существует ли уже значение
    auto existing = m_itemFieldRepo->findByItemAndFieldType(
        *field.itemId, *field.fieldTypeId
    );

    if (existing.has_value())
    {
        // Обновляем существующее
        dto::ItemField updateField;
        updateField.id = existing->id;
        updateField.value = field.value;

        if (!m_itemFieldRepo->update(updateField))
        {
            LOG_ERROR << "setItemField: не удалось обновить значение поля";
            return std::nullopt;
        }

        LOG_INFO
            << "Значение поля обновлено: itemId=" << *field.itemId
            << ", fieldTypeId=" << *field.fieldTypeId
            << ", пользователь=" << userId;

        return m_itemFieldRepo->findById(*existing->id);
    }
    else
    {
        // Создаём новое
        const int64_t newId = m_itemFieldRepo->create(field);
        if (newId <= 0)
        {
            LOG_ERROR << "setItemField: не удалось создать значение поля";
            return std::nullopt;
        }

        LOG_INFO
            << "Значение поля создано: itemId=" << *field.itemId
            << ", fieldTypeId=" << *field.fieldTypeId
            << ", пользователь=" << userId;

        return m_itemFieldRepo->findById(newId);
    }
}

ItemResult ItemService::deleteItemField(
    int64_t itemId,
    int64_t fieldTypeId,
    int64_t userId
)
{
    ItemResult result;

    // 1. Проверяем доступ к элементу и права на запись
    auto item = checkItemAccess(itemId, userId, true);
    if (!item.has_value())
    {
        result.errorMessage = "Элемент не найден или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Проверяем существование значения поля
    auto existing = m_itemFieldRepo->findByItemAndFieldType(itemId, fieldTypeId);
    if (!existing.has_value())
    {
        result.errorMessage = "Значение поля не найдено";
        result.errorCode = 404;
        return result;
    }

    // 3. Удаляем
    if (!m_itemFieldRepo->remove(*existing->id))
    {
        result.errorMessage = "Не удалось удалить значение поля";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Значение поля удалено: itemId=" << itemId
        << ", fieldTypeId=" << fieldTypeId
        << ", пользователь=" << userId;

    return result;
}

// ============================================================
// Приватные методы
// ============================================================

std::optional<int64_t> ItemService::getProjectIdByPhaseId(int64_t phaseId)
{
    auto phase = m_phaseRepo->findById(phaseId);
    if (!phase.has_value() || !phase->projectId.has_value())
    {
        return std::nullopt;
    }
    return *phase->projectId;
}

std::optional<dto::Item> ItemService::checkItemAccess(
    int64_t itemId,
    int64_t userId,
    bool needWrite
)
{
    auto item = m_itemRepo->findById(itemId);
    if (!item.has_value())
    {
        LOG_DEBUG << "checkItemAccess: элемент не найден, id=" << itemId;
        return std::nullopt;
    }

    // Получаем ID проекта
    const auto projectId = getProjectIdByPhaseId(*item->phaseId);
    if (!projectId.has_value())
    {
        LOG_WARN
            << "checkItemAccess: не удалось определить проект для элемента "
            << itemId;
        return std::nullopt;
    }

    // Проверяем права
    AuthzResult authz;
    if (needWrite)
    {
        authz = m_authzService->canWriteToProject(userId, *projectId);
    }
    else
    {
        authz = m_authzService->canReadProject(userId, *projectId);
    }

    if (!authz.granted)
    {
        LOG_DEBUG
            << "checkItemAccess: пользователь " << userId
            << " не имеет доступа к проекту " << *projectId;
        return std::nullopt;
    }

    return item;
}

bool ItemService::checkFieldTypeAccess(
    int64_t fieldTypeId,
    int64_t userId,
    int64_t projectId,
    bool needWrite
)
{
    auto fieldType = m_fieldTypeRepo->findById(fieldTypeId);
    if (!fieldType.has_value() || !fieldType->itemTypeId.has_value())
    {
        LOG_WARN << "checkFieldTypeAccess: тип поля не найден, id=" << fieldTypeId;
        return false;
    }

    AuthzResult authz;
    if (needWrite)
    {
        authz = m_authzService->canWriteItemType(
            userId, projectId, *fieldType->itemTypeId
        );
    }
    else
    {
        authz = m_authzService->canReadItemType(
            userId, projectId, *fieldType->itemTypeId
        );
    }

    return authz.granted;
}

bool ItemService::validateItem(const dto::Item& item, std::string& errorMessage)
{
    if (!item.itemTypeId.has_value())
    {
        errorMessage = "itemTypeId является обязательным полем";
        return false;
    }

    if (!item.stateId.has_value())
    {
        errorMessage = "stateId является обязательным полем";
        return false;
    }

    if (!item.phaseId.has_value())
    {
        errorMessage = "phaseId является обязательным полем";
        return false;
    }

    if (!item.caption.has_value() || item.caption->empty())
    {
        errorMessage = "caption является обязательным полем и не может быть пустым";
        return false;
    }

    if (item.caption->length() > 255)
    {
        errorMessage = "caption не может превышать 255 символов";
        return false;
    }

    if (item.content.has_value() && item.content->length() > 10000)
    {
        errorMessage = "content не может превышать 10000 символов";
        return false;
    }

    return true;
}

bool ItemService::validateItemField(
    const dto::ItemField& field,
    const dto::FieldType& fieldType,
    std::string& errorMessage
)
{
    if (!field.value.has_value())
    {
        // NULL значение допустимо (очистка поля)
        return true;
    }

    const std::string& value = *field.value;
    const std::string& valueType = *fieldType.valueType;

    // Базовая валидация по типу значения
    if (valueType == "String"
        || valueType == "MarkdownText"
        || valueType == "Uri")
    {
        if (value.length() > 10000)
        {
            errorMessage = "Значение не может превышать 10000 символов";
            return false;
        }
    }
    else if (valueType == "Integer")
    {
        try
        {
            std::stoll(value);
        }
        catch (...)
        {
            errorMessage = "Значение должно быть целым числом";
            return false;
        }
    }
    else if (valueType == "Float")
    {
        try
        {
            std::stod(value);
        }
        catch (...)
        {
            errorMessage = "Значение должно быть числом";
            return false;
        }
    }
    else if (valueType == "Bool")
    {
        if (value != "true" && value != "false" && value != "1" && value != "0")
        {
            errorMessage = "Значение должно быть true/false или 1/0";
            return false;
        }
    }
    else if (valueType == "Select")
    {
        // Проверяем, что значение есть в FieldTypePossibleValue
        // TODO: добавить проверку
    }

    return true;
}

std::vector<int64_t> ItemService::getAccessibleProjectIds(int64_t userId)
{
    if (m_authzService->isSuperAdmin(userId))
    {
        // Супер-админ видит все проекты
        return {};
    }

    return m_authzService->getReadableProjectIds(userId);
}

} // namespace services
} // namespace server
