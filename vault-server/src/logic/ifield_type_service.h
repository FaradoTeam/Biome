#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/field_type.h"

namespace server
{
namespace services
{

/**
 * @brief Структура для возврата результатов пагинированного списка типов полей.
 */
struct FieldTypesPage
{
    std::vector<dto::FieldType> fieldTypes;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для управления типами полей.
 */
class IFieldTypeService
{
public:
    virtual ~IFieldTypeService() = default;

    /**
     * @brief Получает список типов полей с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param itemTypeId Фильтр по типу элемента
     * @param valueType Фильтр по типу значения
     * @param searchCaption Поиск по названию
     * @return Страница с типами полей
     */
    virtual FieldTypesPage fieldTypes(
        int page,
        int pageSize,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<std::string> valueType = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает тип поля по ID.
     * @param id Идентификатор типа поля
     * @return DTO типа поля или std::nullopt
     */
    virtual std::optional<dto::FieldType> fieldType(int64_t id) = 0;

    /**
     * @brief Создает новый тип поля.
     * @param fieldType DTO типа поля
     * @param userId ID пользователя для проверки прав
     * @return Созданный тип поля или std::nullopt при ошибке
     */
    virtual std::optional<dto::FieldType> createFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий тип поля.
     * @param fieldType DTO типа поля с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновленный тип поля или std::nullopt при ошибке
     */
    virtual std::optional<dto::FieldType> updateFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет тип поля.
     * @param id Идентификатор типа поля
     * @param userId ID пользователя для проверки прав
     * @return true если удаление успешно
     */
    virtual bool deleteFieldType(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает типы полей для типа элемента.
     * @param itemTypeId Идентификатор типа элемента
     * @return Вектор типов полей
     */
    virtual std::vector<dto::FieldType> fieldTypesByItemType(int64_t itemTypeId) = 0;
};

} // namespace services
} // namespace server
