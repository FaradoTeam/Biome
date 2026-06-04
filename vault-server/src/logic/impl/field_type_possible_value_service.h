#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/ifield_type_possible_value_service.h"

#include "repo/field_type_possible_value_repository.h"
#include "repo/field_type_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для управления возможными значениями полей.
 */
class FieldTypePossibleValueService final : public IFieldTypePossibleValueService
{
public:
    /**
     * @brief Конструктор.
     * @param valueRepo Репозиторий возможных значений полей
     * @param fieldTypeRepo Репозиторий типов полей
     * @param authzService Сервис авторизации для проверки прав
     */
    FieldTypePossibleValueService(
        std::shared_ptr<repositories::IFieldTypePossibleValueRepository> valueRepo,
        std::shared_ptr<repositories::IFieldTypeRepository> fieldTypeRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IFieldTypePossibleValueService
    FieldTypePossibleValuesPage getFieldTypePossibleValues(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) override;

    std::optional<dto::FieldTypePossibleValue> getFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) override;

    std::vector<dto::FieldTypePossibleValue> getValuesByFieldTypeId(
        int64_t fieldTypeId,
        int64_t userId
    ) override;

    std::optional<dto::FieldTypePossibleValue> createFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) override;

    std::optional<dto::FieldTypePossibleValue> updateFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) override;

    FieldTypePossibleValueResult deleteFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет, имеет ли пользователь право на изменение.
     * @param userId ID пользователя
     * @return true если пользователь супер-администратор
     */
    bool canModify(int64_t userId);

    /**
     * @brief Валидирует DTO возможного значения.
     * @param value DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateValue(
        const dto::FieldTypePossibleValue& value,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IFieldTypePossibleValueRepository> m_valueRepo;
    std::shared_ptr<repositories::IFieldTypeRepository> m_fieldTypeRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
