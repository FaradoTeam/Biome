#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/field_type_possible_value.h"

namespace server
{
namespace services
{

/**
 * @brief Структура для возврата результатов пагинированного списка возможных значений полей.
 */
struct FieldTypePossibleValuesPage
{
    std::vector<dto::FieldTypePossibleValue> values;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с возможным значением поля.
 */
struct FieldTypePossibleValueResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для управления возможными значениями полей.
 */
class IFieldTypePossibleValueService
{
public:
    virtual ~IFieldTypePossibleValueService() = default;

    /**
     * @brief Получает список возможных значений с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param fieldTypeId Фильтр по типу поля (опционально)
     * @return Страница с возможными значениями
     */
    virtual FieldTypePossibleValuesPage getFieldTypePossibleValues(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) = 0;

    /**
     * @brief Получает возможное значение по ID.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return DTO возможного значения или std::nullopt
     */
    virtual std::optional<dto::FieldTypePossibleValue> getFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает все возможные значения для указанного типа поля.
     * @param fieldTypeId Идентификатор типа поля
     * @param userId ID пользователя для проверки прав
     * @return Вектор возможных значений
     */
    virtual std::vector<dto::FieldTypePossibleValue> getValuesByFieldTypeId(
        int64_t fieldTypeId,
        int64_t userId
    ) = 0;

    /**
     * @brief Создает новое возможное значение.
     * @param value DTO возможного значения
     * @param userId ID пользователя для проверки прав
     * @return Созданное возможное значение или std::nullopt при ошибке
     */
    virtual std::optional<dto::FieldTypePossibleValue> createFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующее возможное значение.
     * @param value DTO возможного значения с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленное возможное значение или std::nullopt при ошибке
     */
    virtual std::optional<dto::FieldTypePossibleValue> updateFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет возможное значение.
     * @param id Идентификатор записи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual FieldTypePossibleValueResult deleteFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
