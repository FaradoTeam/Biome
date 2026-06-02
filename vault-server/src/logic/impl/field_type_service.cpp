#include "common/log/log.h"

#include "field_type_service.h"

namespace server
{
namespace services
{

FieldTypeService::FieldTypeService(
    std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_fieldTypeRepo(std::move(fieldTypeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_fieldTypeRepo)
    {
        throw std::runtime_error(
            "FieldTypeService: один или несколько репозиториев не инициализированы"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error("AuthorizationService не может быть пустым");
    }
}

FieldTypesPage FieldTypeService::fieldTypes(
    int page,
    int pageSize,
    std::optional<int64_t> itemTypeId,
    std::optional<std::string> valueType,
    const std::string& searchCaption
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [fieldTypeList, total] = m_fieldTypeRepo->findAll(
        page, pageSize, itemTypeId, valueType, searchCaption
    );
    return { fieldTypeList, total };
}

std::optional<dto::FieldType> FieldTypeService::fieldType(int64_t id)
{
    return m_fieldTypeRepo->findById(id);
}

std::optional<dto::FieldType> FieldTypeService::createFieldType(
    const dto::FieldType& fieldType,
    int64_t userId
)
{
    // Только супер-админ может создавать типы полей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "createFieldType: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!fieldType.caption.has_value() || fieldType.caption->empty())
    {
        LOG_WARN << "createFieldType: название типа поля не может быть пустым";
        return std::nullopt;
    }

    if (!fieldType.itemTypeId.has_value())
    {
        LOG_WARN << "createFieldType: не указан itemTypeId";
        return std::nullopt;
    }

    if (!fieldType.valueType.has_value() || fieldType.valueType->empty())
    {
        LOG_WARN << "createFieldType: не указан valueType";
        return std::nullopt;
    }

    const int64_t newId = m_fieldTypeRepo->create(fieldType);
    if (newId <= 0)
    {
        LOG_ERROR << "createFieldType: ошибка при создании типа поля";
        return std::nullopt;
    }

    LOG_INFO << "Создан новый тип поля с id=" << newId << ", пользователь=" << userId;
    return m_fieldTypeRepo->findById(newId);
}

std::optional<dto::FieldType> FieldTypeService::updateFieldType(
    const dto::FieldType& fieldType,
    int64_t userId
)
{
    // Только супер-админ может обновлять типы полей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "updateFieldType: пользователь " << userId << " не имеет прав";
        return std::nullopt;
    }

    if (!fieldType.id.has_value())
    {
        LOG_WARN << "updateFieldType: отсутствует ID типа поля";
        return std::nullopt;
    }

    if (!m_fieldTypeRepo->exists(fieldType.id.value()))
    {
        LOG_WARN << "updateFieldType: тип поля с id=" << fieldType.id.value() << " не найден";
        return std::nullopt;
    }

    if (!m_fieldTypeRepo->update(fieldType))
    {
        LOG_ERROR << "updateFieldType: ошибка при обновлении типа поля";
        return std::nullopt;
    }

    LOG_INFO << "Тип поля с id=" << fieldType.id.value() << " обновлен, пользователь=" << userId;
    return m_fieldTypeRepo->findById(fieldType.id.value());
}

bool FieldTypeService::deleteFieldType(int64_t id, int64_t userId)
{
    // Только супер-админ может удалять типы полей
    if (!m_authzService->isSuperAdmin(userId))
    {
        LOG_WARN << "deleteFieldType: пользователь " << userId << " не имеет прав";
        return false;
    }

    if (!m_fieldTypeRepo->exists(id))
    {
        LOG_WARN << "deleteFieldType: тип поля с id=" << id << " не найден";
        return false;
    }

    if (m_fieldTypeRepo->remove(id))
    {
        LOG_INFO << "Тип поля с id=" << id << " удален, пользователь=" << userId;
        return true;
    }

    LOG_ERROR << "deleteFieldType: ошибка при удалении типа поля с id=" << id;
    return false;
}

std::vector<dto::FieldType> FieldTypeService::fieldTypesByItemType(int64_t itemTypeId)
{
    return m_fieldTypeRepo->findByItemType(itemTypeId);
}

} // namespace services
} // namespace server
