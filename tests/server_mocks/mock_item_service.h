#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/item.h"
#include "common/dto/item_field.h"

#include "logic/iitem_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-реализация сервиса элементов для тестирования.
 */
class MockItemService : public services::IItemService
{
public:
    using ItemsPage = services::ItemsPage;
    using ItemResult = services::ItemResult;

    MockItemService() = default;
    ~MockItemService() override = default;

    // ============================================================
    // Настройка результатов (простой режим)
    // ============================================================

    void setGetItemsResult(const ItemsPage& result)
    {
        m_getItemsResult = result;
        m_getItemsCallback = nullptr;
    }

    void setGetItemResult(std::optional<dto::Item> item)
    {
        m_getItemResult = std::move(item);
        m_getItemCallback = nullptr;
    }

    void setCreateItemResult(std::optional<dto::Item> item)
    {
        m_createItemResult = std::move(item);
        m_createItemCallback = nullptr;
    }

    void setUpdateItemResult(std::optional<dto::Item> item)
    {
        m_updateItemResult = std::move(item);
        m_updateItemCallback = nullptr;
    }

    void setDeleteItemResult(const ItemResult& result)
    {
        m_deleteItemResult = result;
        m_deleteItemCallback = nullptr;
    }

    void setRestoreItemResult(const ItemResult& result)
    {
        m_restoreItemResult = result;
        m_restoreItemCallback = nullptr;
    }

    void setGetItemFieldsResult(std::optional<std::vector<dto::ItemField>> fields)
    {
        m_getItemFieldsResult = std::move(fields);
        m_getItemFieldsCallback = nullptr;
    }

    void setGetItemFieldResult(std::optional<dto::ItemField> field)
    {
        m_getItemFieldResult = std::move(field);
        m_getItemFieldCallback = nullptr;
    }

    void setSetItemFieldResult(std::optional<dto::ItemField> field)
    {
        m_setItemFieldResult = std::move(field);
        m_setItemFieldCallback = nullptr;
    }

    void setDeleteItemFieldResult(const ItemResult& result)
    {
        m_deleteItemFieldResult = result;
        m_deleteItemFieldCallback = nullptr;
    }

    // ============================================================
    // Настройка callback'ов для кастомной логики
    // ============================================================

    void setGetItemsCallback(
        std::function<ItemsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>, std::optional<bool>, const std::string&)> callback
    )
    {
        m_getItemsCallback = std::move(callback);
    }

    void setGetItemCallback(
        std::function<std::optional<dto::Item>(int64_t, int64_t)> callback
    )
    {
        m_getItemCallback = std::move(callback);
    }

    void setCreateItemCallback(
        std::function<std::optional<dto::Item>(const dto::Item&, int64_t)> callback
    )
    {
        m_createItemCallback = std::move(callback);
    }

    void setUpdateItemCallback(
        std::function<std::optional<dto::Item>(const dto::Item&, int64_t)> callback
    )
    {
        m_updateItemCallback = std::move(callback);
    }

    void setDeleteItemCallback(
        std::function<ItemResult(int64_t, int64_t)> callback
    )
    {
        m_deleteItemCallback = std::move(callback);
    }

    void setRestoreItemCallback(
        std::function<ItemResult(int64_t, int64_t)> callback
    )
    {
        m_restoreItemCallback = std::move(callback);
    }

    void setGetItemFieldsCallback(
        std::function<std::optional<std::vector<dto::ItemField>>(int64_t, int64_t)> callback
    )
    {
        m_getItemFieldsCallback = std::move(callback);
    }

    void setGetItemFieldCallback(
        std::function<std::optional<dto::ItemField>(int64_t, int64_t, int64_t)> callback
    )
    {
        m_getItemFieldCallback = std::move(callback);
    }

    void setSetItemFieldCallback(
        std::function<std::optional<dto::ItemField>(const dto::ItemField&, int64_t)> callback
    )
    {
        m_setItemFieldCallback = std::move(callback);
    }

    void setDeleteItemFieldCallback(
        std::function<ItemResult(int64_t, int64_t, int64_t)> callback
    )
    {
        m_deleteItemFieldCallback = std::move(callback);
    }

    // ============================================================
    // Реализация интерфейса IItemService
    // ============================================================

    ItemsPage items(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemTypeId = std::nullopt,
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> stateId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt,
        const std::string& searchCaption = ""
    ) override
    {
        m_lastGetItemsPage = page;
        m_lastGetItemsPageSize = pageSize;
        m_lastGetItemsUserId = userId;
        m_lastGetItemsItemTypeId = itemTypeId;
        m_lastGetItemsParentId = parentId;
        m_lastGetItemsPhaseId = phaseId;
        m_lastGetItemsStateId = stateId;
        m_lastGetItemsIsDeleted = isDeleted;
        m_lastGetItemsSearchCaption = searchCaption;
        ++m_getItemsCallCount;

        if (m_getItemsCallback)
        {
            return m_getItemsCallback(page, pageSize, userId, itemTypeId, parentId, phaseId, stateId, isDeleted, searchCaption);
        }
        return m_getItemsResult;
    }

    std::optional<dto::Item> item(int64_t id, int64_t userId) override
    {
        m_lastGetItemId = id;
        m_lastGetItemUserId = userId;
        ++m_getItemCallCount;

        if (m_getItemCallback)
        {
            return m_getItemCallback(id, userId);
        }

        // Если результат установлен, проверяем соответствие ID
        if (m_getItemResult.has_value() && m_getItemResult->id.has_value())
        {
            if (*m_getItemResult->id == id)
            {
                return m_getItemResult;
            }
        }
        return std::nullopt;
    }

    std::optional<dto::Item> createItem(
        const dto::Item& item,
        int64_t userId
    ) override
    {
        m_lastCreatedItem = item;
        m_lastCreateItemUserId = userId;
        ++m_createItemCallCount;

        if (m_createItemCallback)
        {
            return m_createItemCallback(item, userId);
        }

        // Симуляция проверки прав: обычный пользователь (userId=100) может создавать
        // Пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_createItemResult.has_value())
        {
            // Если ID не установлен, генерируем новый
            if (!m_createItemResult->id.has_value())
            {
                m_createItemResult->id = m_nextItemId++;
            }
        }

        return m_createItemResult;
    }

    std::optional<dto::Item> updateItem(
        const dto::Item& item,
        int64_t userId
    ) override
    {
        m_lastUpdatedItem = item;
        m_lastUpdateItemUserId = userId;
        ++m_updateItemCallCount;

        if (m_updateItemCallback)
        {
            return m_updateItemCallback(item, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        // Проверяем, существует ли элемент
        if (m_updateItemResult.has_value() && m_updateItemResult->id.has_value())
        {
            if (item.id.has_value() && *m_updateItemResult->id == *item.id)
            {
                return m_updateItemResult;
            }
        }

        return std::nullopt;
    }

    ItemResult deleteItem(int64_t id, int64_t userId) override
    {
        m_lastDeletedItemId = id;
        m_lastDeleteItemUserId = userId;
        ++m_deleteItemCallCount;

        if (m_deleteItemCallback)
        {
            return m_deleteItemCallback(id, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет прав
        if (userId == 999)
        {
            ItemResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        return m_deleteItemResult;
    }

    ItemResult restoreItem(int64_t id, int64_t userId) override
    {
        m_lastRestoredItemId = id;
        m_lastRestoreItemUserId = userId;
        ++m_restoreItemCallCount;

        if (m_restoreItemCallback)
        {
            return m_restoreItemCallback(id, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет прав
        if (userId == 999)
        {
            ItemResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        return m_restoreItemResult;
    }

    std::vector<dto::ItemField> getItemFields(
        int64_t itemId,
        int64_t userId
    ) override
    {
        m_lastGetItemFieldsItemId = itemId;
        m_lastGetItemFieldsUserId = userId;
        ++m_getItemFieldsCallCount;

        if (m_getItemFieldsCallback)
        {
            auto result = m_getItemFieldsCallback(itemId, userId);
            if (result.has_value())
            {
                return *result;
            }
            return {};
        }

        if (m_getItemFieldsResult.has_value())
        {
            return *m_getItemFieldsResult;
        }
        return {};
    }

    std::optional<dto::ItemField> getItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) override
    {
        m_lastGetItemFieldItemId = itemId;
        m_lastGetItemFieldFieldTypeId = fieldTypeId;
        m_lastGetItemFieldUserId = userId;
        ++m_getItemFieldCallCount;

        if (m_getItemFieldCallback)
        {
            return m_getItemFieldCallback(itemId, fieldTypeId, userId);
        }

        return m_getItemFieldResult;
    }

    std::optional<dto::ItemField> setItemField(
        const dto::ItemField& field,
        int64_t userId
    ) override
    {
        m_lastSetItemField = field;
        m_lastSetItemFieldUserId = userId;
        ++m_setItemFieldCallCount;

        if (m_setItemFieldCallback)
        {
            return m_setItemFieldCallback(field, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет прав
        if (userId == 999)
        {
            return std::nullopt;
        }

        // Если ID не установлен, генерируем новый
        if (m_setItemFieldResult.has_value() && !m_setItemFieldResult->id.has_value())
        {
            m_setItemFieldResult->id = m_nextFieldId++;
        }

        return m_setItemFieldResult;
    }

    ItemResult deleteItemField(
        int64_t itemId,
        int64_t fieldTypeId,
        int64_t userId
    ) override
    {
        m_lastDeletedItemFieldItemId = itemId;
        m_lastDeletedItemFieldFieldTypeId = fieldTypeId;
        m_lastDeleteItemFieldUserId = userId;
        ++m_deleteItemFieldCallCount;

        if (m_deleteItemFieldCallback)
        {
            return m_deleteItemFieldCallback(itemId, fieldTypeId, userId);
        }

        // Симуляция проверки прав: пользователь 999 не имеет прав
        if (userId == 999)
        {
            ItemResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        return m_deleteItemFieldResult;
    }

    // ============================================================
    // Методы для проверки вызовов
    // ============================================================

    int getGetItemsCallCount() const { return m_getItemsCallCount; }
    int getGetItemCallCount() const { return m_getItemCallCount; }
    int getCreateItemCallCount() const { return m_createItemCallCount; }
    int getUpdateItemCallCount() const { return m_updateItemCallCount; }
    int getDeleteItemCallCount() const { return m_deleteItemCallCount; }
    int getRestoreItemCallCount() const { return m_restoreItemCallCount; }
    int getGetItemFieldsCallCount() const { return m_getItemFieldsCallCount; }
    int getGetItemFieldCallCount() const { return m_getItemFieldCallCount; }
    int getSetItemFieldCallCount() const { return m_setItemFieldCallCount; }
    int getDeleteItemFieldCallCount() const { return m_deleteItemFieldCallCount; }

    // ============================================================
    // Геттеры для последних параметров вызовов
    // ============================================================

    int getLastGetItemsPage() const { return m_lastGetItemsPage; }
    int getLastGetItemsPageSize() const { return m_lastGetItemsPageSize; }
    int64_t getLastGetItemsUserId() const { return m_lastGetItemsUserId; }
    std::optional<int64_t> getLastGetItemsItemTypeId() const { return m_lastGetItemsItemTypeId; }
    std::optional<int64_t> getLastGetItemsParentId() const { return m_lastGetItemsParentId; }
    std::optional<int64_t> getLastGetItemsPhaseId() const { return m_lastGetItemsPhaseId; }
    std::optional<int64_t> getLastGetItemsStateId() const { return m_lastGetItemsStateId; }
    std::optional<bool> getLastGetItemsIsDeleted() const { return m_lastGetItemsIsDeleted; }
    const std::string& getLastGetItemsSearchCaption() const { return m_lastGetItemsSearchCaption; }

    int64_t getLastGetItemId() const { return m_lastGetItemId; }
    int64_t getLastGetItemUserId() const { return m_lastGetItemUserId; }

    const dto::Item& getLastCreatedItem() const { return m_lastCreatedItem; }
    int64_t getLastCreateItemUserId() const { return m_lastCreateItemUserId; }

    const dto::Item& getLastUpdatedItem() const { return m_lastUpdatedItem; }
    int64_t getLastUpdateItemUserId() const { return m_lastUpdateItemUserId; }

    int64_t getLastDeletedItemId() const { return m_lastDeletedItemId; }
    int64_t getLastDeleteItemUserId() const { return m_lastDeleteItemUserId; }

    int64_t getLastRestoredItemId() const { return m_lastRestoredItemId; }
    int64_t getLastRestoreItemUserId() const { return m_lastRestoreItemUserId; }

    int64_t getLastGetItemFieldsItemId() const { return m_lastGetItemFieldsItemId; }
    int64_t getLastGetItemFieldsUserId() const { return m_lastGetItemFieldsUserId; }

    int64_t getLastGetItemFieldItemId() const { return m_lastGetItemFieldItemId; }
    int64_t getLastGetItemFieldFieldTypeId() const { return m_lastGetItemFieldFieldTypeId; }
    int64_t getLastGetItemFieldUserId() const { return m_lastGetItemFieldUserId; }

    const dto::ItemField& getLastSetItemField() const { return m_lastSetItemField; }
    int64_t getLastSetItemFieldUserId() const { return m_lastSetItemFieldUserId; }

    int64_t getLastDeletedItemFieldItemId() const { return m_lastDeletedItemFieldItemId; }
    int64_t getLastDeletedItemFieldFieldTypeId() const { return m_lastDeletedItemFieldFieldTypeId; }
    int64_t getLastDeleteItemFieldUserId() const { return m_lastDeleteItemFieldUserId; }

    // ============================================================
    // Вспомогательные методы
    // ============================================================

    static dto::Item createTestItem(
        int64_t id,
        const std::string& caption,
        int64_t itemTypeId = 1,
        int64_t stateId = 1,
        int64_t phaseId = 1,
        std::optional<int64_t> parentId = std::nullopt,
        bool isDeleted = false
    )
    {
        dto::Item item;
        item.id = id;
        item.caption = caption;
        item.itemTypeId = itemTypeId;
        item.stateId = stateId;
        item.phaseId = phaseId;
        if (parentId.has_value())
            item.parentId = parentId;
        item.isDeleted = isDeleted;
        item.content = "Test content for " + caption;
        return item;
    }

    static dto::ItemField createTestField(
        int64_t id,
        int64_t itemId,
        int64_t fieldTypeId,
        const std::string& value
    )
    {
        dto::ItemField field;
        field.id = id;
        field.itemId = itemId;
        field.fieldTypeId = fieldTypeId;
        field.value = value;
        return field;
    }

    static ItemsPage createTestItemsPage(
        const std::vector<dto::Item>& items,
        int64_t totalCount = -1
    )
    {
        ItemsPage page;
        page.items = items;
        page.totalCount = (totalCount >= 0) ? totalCount : static_cast<int64_t>(items.size());
        return page;
    }

    // ============================================================
    // Сброс состояния
    // ============================================================

    void reset()
    {
        // Сброс счётчиков вызовов
        m_getItemsCallCount = 0;
        m_getItemCallCount = 0;
        m_createItemCallCount = 0;
        m_updateItemCallCount = 0;
        m_deleteItemCallCount = 0;
        m_restoreItemCallCount = 0;
        m_getItemFieldsCallCount = 0;
        m_getItemFieldCallCount = 0;
        m_setItemFieldCallCount = 0;
        m_deleteItemFieldCallCount = 0;

        // Сброс параметров последних вызовов
        m_lastGetItemsPage = 0;
        m_lastGetItemsPageSize = 0;
        m_lastGetItemsUserId = 0;
        m_lastGetItemsItemTypeId = std::nullopt;
        m_lastGetItemsParentId = std::nullopt;
        m_lastGetItemsPhaseId = std::nullopt;
        m_lastGetItemsStateId = std::nullopt;
        m_lastGetItemsIsDeleted = std::nullopt;
        m_lastGetItemsSearchCaption.clear();

        m_lastGetItemId = 0;
        m_lastGetItemUserId = 0;

        m_lastCreatedItem = dto::Item {};
        m_lastCreateItemUserId = 0;

        m_lastUpdatedItem = dto::Item {};
        m_lastUpdateItemUserId = 0;

        m_lastDeletedItemId = 0;
        m_lastDeleteItemUserId = 0;

        m_lastRestoredItemId = 0;
        m_lastRestoreItemUserId = 0;

        m_lastGetItemFieldsItemId = 0;
        m_lastGetItemFieldsUserId = 0;

        m_lastGetItemFieldItemId = 0;
        m_lastGetItemFieldFieldTypeId = 0;
        m_lastGetItemFieldUserId = 0;

        m_lastSetItemField = dto::ItemField {};
        m_lastSetItemFieldUserId = 0;

        m_lastDeletedItemFieldItemId = 0;
        m_lastDeletedItemFieldFieldTypeId = 0;
        m_lastDeleteItemFieldUserId = 0;

        // Сброс callback'ов
        m_getItemsCallback = nullptr;
        m_getItemCallback = nullptr;
        m_createItemCallback = nullptr;
        m_updateItemCallback = nullptr;
        m_deleteItemCallback = nullptr;
        m_restoreItemCallback = nullptr;
        m_getItemFieldsCallback = nullptr;
        m_getItemFieldCallback = nullptr;
        m_setItemFieldCallback = nullptr;
        m_deleteItemFieldCallback = nullptr;

        // Сброс результатов
        m_getItemsResult = ItemsPage {};
        m_getItemResult = std::nullopt;
        m_createItemResult = std::nullopt;
        m_updateItemResult = std::nullopt;
        m_deleteItemResult = ItemResult { false, 0, "" };
        m_restoreItemResult = ItemResult { false, 0, "" };
        m_getItemFieldsResult = std::nullopt;
        m_getItemFieldResult = std::nullopt;
        m_setItemFieldResult = std::nullopt;
        m_deleteItemFieldResult = ItemResult { false, 0, "" };

        // Сброс генераторов ID
        m_nextItemId = 100;
        m_nextFieldId = 100;
    }

private:
    // Предустановленные результаты
    ItemsPage m_getItemsResult;
    std::optional<dto::Item> m_getItemResult;
    std::optional<dto::Item> m_createItemResult;
    std::optional<dto::Item> m_updateItemResult;
    ItemResult m_deleteItemResult;
    ItemResult m_restoreItemResult;
    std::optional<std::vector<dto::ItemField>> m_getItemFieldsResult;
    std::optional<dto::ItemField> m_getItemFieldResult;
    std::optional<dto::ItemField> m_setItemFieldResult;
    ItemResult m_deleteItemFieldResult;

    // Генераторы ID для тестов
    int64_t m_nextItemId = 100;
    int64_t m_nextFieldId = 100;

    // Callback'и для кастомной логики
    std::function<ItemsPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>, std::optional<bool>, const std::string&)> m_getItemsCallback;
    std::function<std::optional<dto::Item>(int64_t, int64_t)> m_getItemCallback;
    std::function<std::optional<dto::Item>(const dto::Item&, int64_t)> m_createItemCallback;
    std::function<std::optional<dto::Item>(const dto::Item&, int64_t)> m_updateItemCallback;
    std::function<ItemResult(int64_t, int64_t)> m_deleteItemCallback;
    std::function<ItemResult(int64_t, int64_t)> m_restoreItemCallback;
    std::function<std::optional<std::vector<dto::ItemField>>(int64_t, int64_t)> m_getItemFieldsCallback;
    std::function<std::optional<dto::ItemField>(int64_t, int64_t, int64_t)> m_getItemFieldCallback;
    std::function<std::optional<dto::ItemField>(const dto::ItemField&, int64_t)> m_setItemFieldCallback;
    std::function<ItemResult(int64_t, int64_t, int64_t)> m_deleteItemFieldCallback;

    // Счётчики вызовов
    int m_getItemsCallCount = 0;
    int m_getItemCallCount = 0;
    int m_createItemCallCount = 0;
    int m_updateItemCallCount = 0;
    int m_deleteItemCallCount = 0;
    int m_restoreItemCallCount = 0;
    int m_getItemFieldsCallCount = 0;
    int m_getItemFieldCallCount = 0;
    int m_setItemFieldCallCount = 0;
    int m_deleteItemFieldCallCount = 0;

    // Параметры последних вызовов - GET /api/items
    int m_lastGetItemsPage = 0;
    int m_lastGetItemsPageSize = 0;
    int64_t m_lastGetItemsUserId = 0;
    std::optional<int64_t> m_lastGetItemsItemTypeId;
    std::optional<int64_t> m_lastGetItemsParentId;
    std::optional<int64_t> m_lastGetItemsPhaseId;
    std::optional<int64_t> m_lastGetItemsStateId;
    std::optional<bool> m_lastGetItemsIsDeleted;
    std::string m_lastGetItemsSearchCaption;

    // Параметры последних вызовов - GET /api/items/{id}
    int64_t m_lastGetItemId = 0;
    int64_t m_lastGetItemUserId = 0;

    // Параметры последних вызовов - POST /api/items
    dto::Item m_lastCreatedItem;
    int64_t m_lastCreateItemUserId = 0;

    // Параметры последних вызовов - PUT /api/items/{id}
    dto::Item m_lastUpdatedItem;
    int64_t m_lastUpdateItemUserId = 0;

    // Параметры последних вызовов - DELETE /api/items/{id}
    int64_t m_lastDeletedItemId = 0;
    int64_t m_lastDeleteItemUserId = 0;

    // Параметры последних вызовов - POST /api/items/{id}/restore
    int64_t m_lastRestoredItemId = 0;
    int64_t m_lastRestoreItemUserId = 0;

    // Параметры последних вызовов - GET /api/items/{id}/fields
    int64_t m_lastGetItemFieldsItemId = 0;
    int64_t m_lastGetItemFieldsUserId = 0;

    // Параметры последних вызовов - GET /api/items/{id}/fields/{fieldTypeId}
    int64_t m_lastGetItemFieldItemId = 0;
    int64_t m_lastGetItemFieldFieldTypeId = 0;
    int64_t m_lastGetItemFieldUserId = 0;

    // Параметры последних вызовов - PUT /api/items/{id}/fields/{fieldTypeId}
    dto::ItemField m_lastSetItemField;
    int64_t m_lastSetItemFieldUserId = 0;

    // Параметры последних вызовов - DELETE /api/items/{id}/fields/{fieldTypeId}
    int64_t m_lastDeletedItemFieldItemId = 0;
    int64_t m_lastDeletedItemFieldFieldTypeId = 0;
    int64_t m_lastDeleteItemFieldUserId = 0;
};

} // namespace tests
} // namespace server
