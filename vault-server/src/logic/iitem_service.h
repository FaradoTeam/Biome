#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/item.h"
#include "common/dto/item_field.h"

namespace server
{
namespace services
{

/**
 * @brief Результат операции с элементом.
 */
struct ItemResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Страница с элементами.
 */
struct ItemsPage
{
    std::vector<dto::Item> items;
    int64_t totalCount = 0;
};

/**
 * @brief Страница со значениями полей.
 */
struct ItemFieldsPage
{
    std::vector<dto::ItemField> fields;
    int64_t totalCount = 0;
};

/**
 * @brief Интерфейс сервиса для работы с элементами.
 */
class IItemService
{
public:
    virtual ~IItemService() = default;

    /**
     * @brief Получает список элементов с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param itemTypeId Фильтр по типу элемента
     * @param parentId Фильтр по родительскому элементу
     * @param phaseId Фильтр по фазе
     * @param stateId Фильтр по состоянию
     * @param isDeleted Фильтр по статусу удаления
     * @param searchCaption Поиск по названию
     * @return Страница с элементами
     */
    virtual ItemsPage items(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt,
        const std::string& searchCaption = ""
    ) = 0;

    /**
     * @brief Получает элемент по ID.
     * @param id Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return DTO элемента или std::nullopt
     */
    virtual std::optional<dto::Item> item(int64_t id, int64_t userId) = 0;

    /**
     * @brief Создаёт новый элемент.
     * @param item DTO элемента
     * @param userId ID пользователя для проверки прав
     * @return Созданный элемент или std::nullopt при ошибке
     */
    virtual std::optional<dto::Item> createItem(
        const dto::Item& item,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующий элемент.
     * @param item DTO элемента с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённый элемент или std::nullopt при ошибке
     */
    virtual std::optional<dto::Item> updateItem(
        const dto::Item& item,
        int64_t userId
    ) = 0;

    /**
     * @brief Мягкое удаление элемента.
     * @param id Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemResult deleteItem(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Восстанавливает элемент из мягкого удаления.
     * @param id Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemResult restoreItem(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает значения всех полей элемента.
     * @param itemId Идентификатор элемента
     * @param userId ID пользователя для проверки прав
     * @return Вектор значений полей
     */
    virtual std::vector<dto::ItemField> getItemFields(
        int64_t itemId,
        int64_t userId
    ) = 0;

    /**
     * @brief Получает значение конкретного поля элемента.
     * @param itemId Идентификатор элемента
     * @param fieldTypeId Идентификатор типа поля
     * @param userId ID пользователя для проверки прав
     * @return Значение поля или std::nullopt
     */
    virtual std::optional<dto::ItemField> getItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) = 0;

    /**
     * @brief Устанавливает значение поля элемента.
     * @param field Значение поля
     * @param userId ID пользователя для проверки прав
     * @return Созданное/обновлённое значение поля или std::nullopt при ошибке
     */
    virtual std::optional<dto::ItemField> setItemField(
        const dto::ItemField& field,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет значение поля элемента.
     * @param itemId Идентификатор элемента
     * @param fieldTypeId Идентификатор типа поля
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual ItemResult deleteItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
