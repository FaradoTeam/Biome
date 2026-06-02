#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/item_type.h"

#include "logic/iitem_type_service.h"

namespace server
{
namespace tests
{

class MockItemTypeService : public services::IItemTypeService
{
public:
    using ItemTypesPage = services::ItemTypesPage;

    void setGetItemTypesResult(const ItemTypesPage& result)
    {
        m_getItemTypesResult = result;
        m_getItemTypesCallback = nullptr;
    }

    void setGetItemTypeResult(std::optional<dto::ItemType> itemType)
    {
        m_getItemTypeResult = std::move(itemType);
        m_getItemTypeCallback = nullptr;
    }

    void setCreateItemTypeResult(std::optional<dto::ItemType> itemType)
    {
        m_createItemTypeResult = std::move(itemType);
        m_createItemTypeCallback = nullptr;
    }

    void setUpdateItemTypeResult(std::optional<dto::ItemType> itemType)
    {
        m_updateItemTypeResult = std::move(itemType);
        m_updateItemTypeCallback = nullptr;
    }

    void setDeleteItemTypeResult(bool result)
    {
        m_deleteItemTypeResult = result;
        m_deleteItemTypeCallback = nullptr;
    }

    void setItemTypesByWorkflowResult(std::vector<dto::ItemType> itemTypes)
    {
        m_itemTypesByWorkflowResult = std::move(itemTypes);
    }

    // Callback-и
    void setGetItemTypesCallback(
        std::function<ItemTypesPage(int, int, std::optional<int64_t>, std::optional<std::string>, const std::string&)> callback
    )
    {
        m_getItemTypesCallback = std::move(callback);
    }

    void setGetItemTypeCallback(
        std::function<std::optional<dto::ItemType>(int64_t)> callback
    )
    {
        m_getItemTypeCallback = std::move(callback);
    }

    void setCreateItemTypeCallback(
        std::function<std::optional<dto::ItemType>(const dto::ItemType&, int64_t)> callback
    )
    {
        m_createItemTypeCallback = std::move(callback);
    }

    void setUpdateItemTypeCallback(
        std::function<std::optional<dto::ItemType>(const dto::ItemType&, int64_t)> callback
    )
    {
        m_updateItemTypeCallback = std::move(callback);
    }

    void setDeleteItemTypeCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteItemTypeCallback = std::move(callback);
    }

    // Реализация интерфейса
    ItemTypesPage itemTypes(
        int page,
        int pageSize,
        std::optional<int64_t> workflowId,
        std::optional<std::string> kind,
        const std::string& searchCaption
    ) override
    {
        m_lastGetItemTypesPage = page;
        m_lastGetItemTypesPageSize = pageSize;
        m_lastGetItemTypesWorkflowId = workflowId;
        m_lastGetItemTypesKind = kind;
        m_lastGetItemTypesSearch = searchCaption;
        m_getItemTypesCallCount++;

        if (m_getItemTypesCallback)
        {
            return m_getItemTypesCallback(page, pageSize, workflowId, kind, searchCaption);
        }
        return m_getItemTypesResult;
    }

    std::optional<dto::ItemType> itemType(int64_t id) override
    {
        m_lastGetItemTypeId = id;
        m_getItemTypeCallCount++;

        if (m_getItemTypeCallback)
        {
            return m_getItemTypeCallback(id);
        }
        return m_getItemTypeResult;
    }

    std::optional<dto::ItemType> createItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) override
    {
        m_lastCreatedItemType = itemType;
        m_lastCreateItemTypeUserId = userId;
        m_createItemTypeCallCount++;

        if (m_createItemTypeCallback)
        {
            return m_createItemTypeCallback(itemType, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createItemTypeResult;
    }

    std::optional<dto::ItemType> updateItemType(
        const dto::ItemType& itemType,
        int64_t userId
    ) override
    {
        m_lastUpdatedItemType = itemType;
        m_lastUpdateItemTypeUserId = userId;
        m_updateItemTypeCallCount++;

        if (m_updateItemTypeCallback)
        {
            return m_updateItemTypeCallback(itemType, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateItemTypeResult;
    }

    bool deleteItemType(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedItemTypeId = id;
        m_lastDeleteItemTypeUserId = userId;
        m_deleteItemTypeCallCount++;

        if (m_deleteItemTypeCallback)
        {
            return m_deleteItemTypeCallback(id, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteItemTypeResult;
    }

    std::vector<dto::ItemType> itemTypesByWorkflow(int64_t workflowId) override
    {
        m_lastItemTypesByWorkflowId = workflowId;
        m_itemTypesByWorkflowCallCount++;
        return m_itemTypesByWorkflowResult;
    }

    // Методы для проверки вызовов
    int getItemTypesCallCount() const { return m_getItemTypesCallCount; }
    int getItemTypeCallCount() const { return m_getItemTypeCallCount; }
    int createItemTypeCallCount() const { return m_createItemTypeCallCount; }
    int updateItemTypeCallCount() const { return m_updateItemTypeCallCount; }
    int deleteItemTypeCallCount() const { return m_deleteItemTypeCallCount; }

    int getLastGetItemTypesPage() const { return m_lastGetItemTypesPage; }
    int64_t getLastGetItemTypeId() const { return m_lastGetItemTypeId; }
    const dto::ItemType& getLastCreatedItemType() const { return m_lastCreatedItemType; }
    int64_t getLastCreateItemTypeUserId() const { return m_lastCreateItemTypeUserId; }
    const dto::ItemType& getLastUpdatedItemType() const { return m_lastUpdatedItemType; }
    int64_t getLastUpdateItemTypeUserId() const { return m_lastUpdateItemTypeUserId; }
    int64_t getLastDeletedItemTypeId() const { return m_lastDeletedItemTypeId; }
    int64_t getLastDeleteItemTypeUserId() const { return m_lastDeleteItemTypeUserId; }

    void reset()
    {
        m_getItemTypesCallCount = 0;
        m_getItemTypeCallCount = 0;
        m_createItemTypeCallCount = 0;
        m_updateItemTypeCallCount = 0;
        m_deleteItemTypeCallCount = 0;
        m_itemTypesByWorkflowCallCount = 0;

        m_lastGetItemTypesPage = 0;
        m_lastGetItemTypesPageSize = 0;
        m_lastGetItemTypeId = 0;
        m_lastDeletedItemTypeId = 0;
        m_lastCreateItemTypeUserId = 0;
        m_lastUpdateItemTypeUserId = 0;
        m_lastDeleteItemTypeUserId = 0;
    }

private:
    ItemTypesPage m_getItemTypesResult;
    std::optional<dto::ItemType> m_getItemTypeResult;
    std::optional<dto::ItemType> m_createItemTypeResult;
    std::optional<dto::ItemType> m_updateItemTypeResult;
    bool m_deleteItemTypeResult = false;
    std::vector<dto::ItemType> m_itemTypesByWorkflowResult;

    // Callback-и
    std::function<ItemTypesPage(int, int, std::optional<int64_t>, std::optional<std::string>, const std::string&)> m_getItemTypesCallback;
    std::function<std::optional<dto::ItemType>(int64_t)> m_getItemTypeCallback;
    std::function<std::optional<dto::ItemType>(const dto::ItemType&, int64_t)> m_createItemTypeCallback;
    std::function<std::optional<dto::ItemType>(const dto::ItemType&, int64_t)> m_updateItemTypeCallback;
    std::function<bool(int64_t, int64_t)> m_deleteItemTypeCallback;

    int m_getItemTypesCallCount = 0;
    int m_getItemTypeCallCount = 0;
    int m_createItemTypeCallCount = 0;
    int m_updateItemTypeCallCount = 0;
    int m_deleteItemTypeCallCount = 0;
    int m_itemTypesByWorkflowCallCount = 0;

    int m_lastGetItemTypesPage = 0;
    int m_lastGetItemTypesPageSize = 0;
    std::optional<int64_t> m_lastGetItemTypesWorkflowId;
    std::optional<std::string> m_lastGetItemTypesKind;
    std::string m_lastGetItemTypesSearch;
    int64_t m_lastGetItemTypeId = 0;
    dto::ItemType m_lastCreatedItemType;
    int64_t m_lastCreateItemTypeUserId = 0;
    dto::ItemType m_lastUpdatedItemType;
    int64_t m_lastUpdateItemTypeUserId = 0;
    int64_t m_lastDeletedItemTypeId = 0;
    int64_t m_lastDeleteItemTypeUserId = 0;
    int64_t m_lastItemTypesByWorkflowId = 0;
};

} // namespace tests
} // namespace server
