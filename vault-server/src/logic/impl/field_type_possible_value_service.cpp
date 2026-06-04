#include "common/log/log.h"

#include "field_type_possible_value_service.h"

namespace server
{
namespace services
{

FieldTypePossibleValueService::FieldTypePossibleValueService(
    std::shared_ptr<repositories::IFieldTypePossibleValueRepository> valueRepo,
    std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_valueRepo(std::move(valueRepo))
    , m_fieldTypeRepo(std::move(fieldTypeRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_valueRepo)
    {
        throw std::runtime_error(
            "FieldTypePossibleValueService: репозиторий значений не инициализирован"
        );
    }
    if (!m_fieldTypeRepo)
    {
        throw std::runtime_error(
            "FieldTypePossibleValueService: репозиторий типов полей не инициализирован"
        );
    }
    if (!m_authzService)
    {
        throw std::runtime_error(
            "FieldTypePossibleValueService: сервис авторизации не инициализирован"
        );
    }
}

FieldTypePossibleValuesPage FieldTypePossibleValueService::getFieldTypePossibleValues(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> fieldTypeId
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;

    auto [values, total] = m_valueRepo->findAll(page, pageSize, fieldTypeId);
    return { values, total };
}

std::optional<dto::FieldTypePossibleValue>
FieldTypePossibleValueService::getFieldTypePossibleValue(
    int64_t id,
    int64_t userId
)
{
    if (id <= 0)
    {
        LOG_WARN << "getFieldTypePossibleValue: неверный ID " << id;
        return std::nullopt;
    }

    auto value = m_valueRepo->findById(id);
    if (!value.has_value())
    {
        LOG_DEBUG << "getFieldTypePossibleValue: значение с id=" << id << " не найдено";
        return std::nullopt;
    }

    // Проверяем доступ к типу поля
    if (!value->fieldTypeId.has_value())
    {
        LOG_WARN << "getFieldTypePossibleValue: значение " << id << " не имеет fieldTypeId";
        return std::nullopt;
    }

    return value;
}

std::vector<dto::FieldTypePossibleValue>
FieldTypePossibleValueService::getValuesByFieldTypeId(
    int64_t fieldTypeId,
    int64_t userId
)
{
    if (fieldTypeId <= 0)
    {
        LOG_WARN << "getValuesByFieldTypeId: неверный fieldTypeId " << fieldTypeId;
        return {};
    }

    return m_valueRepo->findByFieldTypeId(fieldTypeId);
}

std::optional<dto::FieldTypePossibleValue>
FieldTypePossibleValueService::createFieldTypePossibleValue(
    const dto::FieldTypePossibleValue& value,
    int64_t userId
)
{
    // Только супер-админ может создавать возможные значения
    if (!canModify(userId))
    {
        LOG_WARN
            << "createFieldTypePossibleValue: пользователь " << userId
            << " не имеет прав на создание возможных значений";
        return std::nullopt;
    }

    // Валидация
    std::string errorMessage;
    if (!validateValue(value, errorMessage))
    {
        LOG_WARN << "createFieldTypePossibleValue: " << errorMessage;
        return std::nullopt;
    }

    if (!value.fieldTypeId.has_value())
    {
        LOG_WARN << "createFieldTypePossibleValue: отсутствует fieldTypeId";
        return std::nullopt;
    }

    // Проверяем существование типа поля
    if (!m_fieldTypeRepo->exists(*value.fieldTypeId))
    {
        LOG_WARN
            << "createFieldTypePossibleValue: тип поля с id="
            << *value.fieldTypeId << " не найден";
        return std::nullopt;
    }

    // Проверяем уникальность значения для данного типа поля
    if (m_valueRepo->existsByValue(*value.fieldTypeId, *value.value))
    {
        LOG_WARN
            << "createFieldTypePossibleValue: значение '"
            << *value.value << "' уже существует для типа поля "
            << *value.fieldTypeId;
        return std::nullopt;
    }

    const int64_t newId = m_valueRepo->create(value);
    if (newId <= 0)
    {
        LOG_ERROR << "createFieldTypePossibleValue: не удалось создать значение";
        return std::nullopt;
    }

    LOG_INFO
        << "Создано возможное значение: id=" << newId
        << ", fieldTypeId=" << *value.fieldTypeId
        << ", value='" << *value.value << "'"
        << ", пользователь=" << userId;

    return m_valueRepo->findById(newId);
}

std::optional<dto::FieldTypePossibleValue>
FieldTypePossibleValueService::updateFieldTypePossibleValue(
    const dto::FieldTypePossibleValue& value,
    int64_t userId
)
{
    // Только супер-админ может обновлять возможные значения
    if (!canModify(userId))
    {
        LOG_WARN
            << "updateFieldTypePossibleValue: пользователь " << userId
            << " не имеет прав на обновление возможных значений";
        return std::nullopt;
    }

    if (!value.id.has_value())
    {
        LOG_WARN << "updateFieldTypePossibleValue: отсутствует ID";
        return std::nullopt;
    }

    auto existing = m_valueRepo->findById(*value.id);
    if (!existing.has_value())
    {
        LOG_WARN
            << "updateFieldTypePossibleValue: значение с id="
            << *value.id << " не найдено";
        return std::nullopt;
    }

    // Если меняется fieldTypeId, проверяем существование нового типа поля
    if (value.fieldTypeId.has_value() && *value.fieldTypeId != *existing->fieldTypeId)
    {
        if (!m_fieldTypeRepo->exists(*value.fieldTypeId))
        {
            LOG_WARN
                << "updateFieldTypePossibleValue: новый fieldTypeId="
                << *value.fieldTypeId << " не найден";
            return std::nullopt;
        }
    }

    // Если меняется значение, проверяем уникальность
    if (value.value.has_value() && *value.value != *existing->value)
    {
        int64_t effectiveFieldTypeId = value.fieldTypeId.has_value()
            ? *value.fieldTypeId
            : *existing->fieldTypeId;

        if (m_valueRepo->existsByValue(effectiveFieldTypeId, *value.value))
        {
            LOG_WARN
                << "updateFieldTypePossibleValue: значение '"
                << *value.value << "' уже существует для типа поля "
                << effectiveFieldTypeId;
            return std::nullopt;
        }
    }

    if (!m_valueRepo->update(value))
    {
        LOG_ERROR
            << "updateFieldTypePossibleValue: не удалось обновить значение id="
            << *value.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Обновлено возможное значение: id=" << *value.id
        << ", пользователь=" << userId;

    return m_valueRepo->findById(*value.id);
}

FieldTypePossibleValueResult
FieldTypePossibleValueService::deleteFieldTypePossibleValue(
    int64_t id,
    int64_t userId
)
{
    FieldTypePossibleValueResult result;

    // Только супер-админ может удалять возможные значения
    if (!canModify(userId))
    {
        result.errorMessage = "Недостаточно прав для удаления возможного значения";
        result.errorCode = 403;
        LOG_WARN
            << "deleteFieldTypePossibleValue: пользователь " << userId
            << " не имеет прав на удаление";
        return result;
    }

    if (id <= 0)
    {
        result.errorMessage = "Неверный идентификатор";
        result.errorCode = 400;
        return result;
    }

    auto existing = m_valueRepo->findById(id);
    if (!existing.has_value())
    {
        result.errorMessage = "Возможное значение не найдено";
        result.errorCode = 404;
        return result;
    }

    if (!m_valueRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить возможное значение";
        result.errorCode = 500;
        LOG_ERROR
            << "deleteFieldTypePossibleValue: ошибка удаления значения id=" << id;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Удалено возможное значение: id=" << id
        << ", пользователь=" << userId;
    return result;
}

bool FieldTypePossibleValueService::canModify(int64_t userId)
{
    return m_authzService->isSuperAdmin(userId);
}

bool FieldTypePossibleValueService::validateValue(
    const dto::FieldTypePossibleValue& value,
    std::string& errorMessage
)
{
    if (!value.fieldTypeId.has_value())
    {
        errorMessage = "fieldTypeId является обязательным полем";
        return false;
    }

    if (!value.value.has_value() || value.value->empty())
    {
        errorMessage = "value является обязательным полем и не может быть пустым";
        return false;
    }

    if (value.value->length() > 255)
    {
        errorMessage = "значение не может превышать 255 символов";
        return false;
    }

    return true;
}

} // namespace services
} // namespace server
