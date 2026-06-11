#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/item_link.h"
#include "logic/iitem_link_service.h"

namespace server::tests
{

class MockItemLinkService : public services::IItemLinkService
{
public:
    using ItemLinksPage = services::ItemLinksPage;
    using ItemLinkResult = services::ItemLinkResult;

    void setGetItemLinksResult(const ItemLinksPage& result)
    {
        m_getItemLinksResult = result;
        m_getItemLinksCallback = nullptr;
    }

    void setGetItemLinkResult(std::optional<dto::ItemLink> link)
    {
        m_getItemLinkResult = std::move(link);
        m_getItemLinkCallback = nullptr;
    }

    void setCreateItemLinkResult(std::optional<dto::ItemLink> link)
    {
        m_createItemLinkResult = std::move(link);
        m_createItemLinkCallback = nullptr;
    }

    void setDeleteItemLinkResult(const ItemLinkResult& result)
    {
        m_deleteItemLinkResult = result;
        m_deleteItemLinkCallback = nullptr;
    }

    void setItemLinksByItemIdResult(const std::vector<dto::ItemLink>& links)
    {
        m_itemLinksByItemIdResult = links;
        m_itemLinksByItemIdCallback = nullptr;
    }

    void setItemLinksByLinkTypeIdResult(const std::vector<dto::ItemLink>& links)
    {
        m_itemLinksByLinkTypeIdResult = links;
        m_itemLinksByLinkTypeIdCallback = nullptr;
    }

    // Callback-и
    void setGetItemLinksCallback(
        std::function<ItemLinksPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_getItemLinksCallback = std::move(callback);
    }

    void setGetItemLinkCallback(
        std::function<std::optional<dto::ItemLink>(int64_t, int64_t)> callback
    )
    {
        m_getItemLinkCallback = std::move(callback);
    }

    void setCreateItemLinkCallback(
        std::function<std::optional<dto::ItemLink>(const dto::ItemLink&, int64_t)> callback
    )
    {
        m_createItemLinkCallback = std::move(callback);
    }

    void setDeleteItemLinkCallback(
        std::function<ItemLinkResult(int64_t, int64_t)> callback
    )
    {
        m_deleteItemLinkCallback = std::move(callback);
    }

    // Реализация интерфейса
    ItemLinksPage getItemLinks(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> linkTypeId = std::nullopt,
        std::optional<int64_t> sourceItemId = std::nullopt,
        std::optional<int64_t> destinationItemId = std::nullopt
    ) override
    {
        m_lastGetItemLinksPage = page;
        m_lastGetItemLinksPageSize = pageSize;
        m_lastGetItemLinksUserId = userId;
        m_lastGetItemLinksLinkTypeId = linkTypeId;
        m_lastGetItemLinksSourceItemId = sourceItemId;
        m_lastGetItemLinksDestItemId = destinationItemId;
        ++m_getItemLinksCallCount;

        if (m_getItemLinksCallback)
        {
            return m_getItemLinksCallback(page, pageSize, userId, linkTypeId, sourceItemId, destinationItemId);
        }
        return m_getItemLinksResult;
    }

    std::optional<dto::ItemLink> getItemLink(int64_t id, int64_t userId) override
    {
        m_lastGetItemLinkId = id;
        m_lastGetItemLinkUserId = userId;
        ++m_getItemLinkCallCount;

        if (m_getItemLinkCallback)
        {
            return m_getItemLinkCallback(id, userId);
        }
        return m_getItemLinkResult;
    }

    std::vector<dto::ItemLink> getItemLinksByItemId(int64_t itemId, int64_t userId) override
    {
        m_lastGetItemLinksByItemId = itemId;
        m_lastGetItemLinksByItemIdUserId = userId;
        ++m_getItemLinksByItemIdCallCount;

        if (m_itemLinksByItemIdCallback)
        {
            return m_itemLinksByItemIdCallback(itemId, userId);
        }
        return m_itemLinksByItemIdResult;
    }

    std::vector<dto::ItemLink> getItemLinksByLinkTypeId(int64_t linkTypeId, int64_t userId) override
    {
        m_lastGetItemLinksByLinkTypeId = linkTypeId;
        m_lastGetItemLinksByLinkTypeIdUserId = userId;
        ++m_getItemLinksByLinkTypeIdCallCount;

        if (m_itemLinksByLinkTypeIdCallback)
        {
            return m_itemLinksByLinkTypeIdCallback(linkTypeId, userId);
        }
        return m_itemLinksByLinkTypeIdResult;
    }

    std::optional<dto::ItemLink> createItemLink(
        const dto::ItemLink& link,
        int64_t userId
    ) override
    {
        m_lastCreatedItemLink = link;
        m_lastCreateItemLinkUserId = userId;
        ++m_createItemLinkCallCount;

        if (m_createItemLinkCallback)
        {
            return m_createItemLinkCallback(link, userId);
        }

        if (userId == 999)
        {
            return std::nullopt;
        }

        if (m_createItemLinkResult.has_value() && !m_createItemLinkResult->id.has_value())
        {
            m_createItemLinkResult->id = m_nextId++;
        }
        return m_createItemLinkResult;
    }

    ItemLinkResult deleteItemLink(int64_t id, int64_t userId) override
    {
        m_lastDeletedItemLinkId = id;
        m_lastDeleteItemLinkUserId = userId;
        ++m_deleteItemLinkCallCount;

        if (m_deleteItemLinkCallback)
        {
            return m_deleteItemLinkCallback(id, userId);
        }

        if (userId == 999)
        {
            ItemLinkResult result;
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }
        return m_deleteItemLinkResult;
    }

    // Методы для проверки вызовов
    int getGetItemLinksCallCount() const { return m_getItemLinksCallCount; }
    int getGetItemLinkCallCount() const { return m_getItemLinkCallCount; }
    int getGetItemLinksByItemIdCallCount() const { return m_getItemLinksByItemIdCallCount; }
    int getGetItemLinksByLinkTypeIdCallCount() const { return m_getItemLinksByLinkTypeIdCallCount; }
    int getCreateItemLinkCallCount() const { return m_createItemLinkCallCount; }
    int getDeleteItemLinkCallCount() const { return m_deleteItemLinkCallCount; }

    int getLastGetItemLinksPage() const { return m_lastGetItemLinksPage; }
    int getLastGetItemLinksPageSize() const { return m_lastGetItemLinksPageSize; }
    int64_t getLastGetItemLinksUserId() const { return m_lastGetItemLinksUserId; }
    std::optional<int64_t> getLastGetItemLinksLinkTypeId() const { return m_lastGetItemLinksLinkTypeId; }
    std::optional<int64_t> getLastGetItemLinksSourceItemId() const { return m_lastGetItemLinksSourceItemId; }
    std::optional<int64_t> getLastGetItemLinksDestItemId() const { return m_lastGetItemLinksDestItemId; }
    int64_t getLastGetItemLinkId() const { return m_lastGetItemLinkId; }
    int64_t getLastGetItemLinkUserId() const { return m_lastGetItemLinkUserId; }
    int64_t getLastGetItemLinksByItemId() const { return m_lastGetItemLinksByItemId; }
    int64_t getLastGetItemLinksByLinkTypeId() const { return m_lastGetItemLinksByLinkTypeId; }
    const dto::ItemLink& getLastCreatedItemLink() const { return m_lastCreatedItemLink; }
    int64_t getLastCreateItemLinkUserId() const { return m_lastCreateItemLinkUserId; }
    int64_t getLastDeletedItemLinkId() const { return m_lastDeletedItemLinkId; }
    int64_t getLastDeleteItemLinkUserId() const { return m_lastDeleteItemLinkUserId; }

    void reset()
    {
        m_getItemLinksCallCount = 0;
        m_getItemLinkCallCount = 0;
        m_getItemLinksByItemIdCallCount = 0;
        m_getItemLinksByLinkTypeIdCallCount = 0;
        m_createItemLinkCallCount = 0;
        m_deleteItemLinkCallCount = 0;

        m_lastGetItemLinksPage = 0;
        m_lastGetItemLinksPageSize = 0;
        m_lastGetItemLinksUserId = 0;
        m_lastGetItemLinksLinkTypeId.reset();
        m_lastGetItemLinksSourceItemId.reset();
        m_lastGetItemLinksDestItemId.reset();
        m_lastGetItemLinkId = 0;
        m_lastGetItemLinkUserId = 0;
        m_lastGetItemLinksByItemId = 0;
        m_lastGetItemLinksByLinkTypeId = 0;
        m_lastCreatedItemLink = dto::ItemLink {};
        m_lastCreateItemLinkUserId = 0;
        m_lastDeletedItemLinkId = 0;
        m_lastDeleteItemLinkUserId = 0;

        m_nextId = 100;
    }

private:
    ItemLinksPage m_getItemLinksResult;
    std::optional<dto::ItemLink> m_getItemLinkResult;
    std::optional<dto::ItemLink> m_createItemLinkResult;
    ItemLinkResult m_deleteItemLinkResult;
    std::vector<dto::ItemLink> m_itemLinksByItemIdResult;
    std::vector<dto::ItemLink> m_itemLinksByLinkTypeIdResult;

    std::function<ItemLinksPage(int, int, int64_t, std::optional<int64_t>, std::optional<int64_t>, std::optional<int64_t>)> m_getItemLinksCallback;
    std::function<std::optional<dto::ItemLink>(int64_t, int64_t)> m_getItemLinkCallback;
    std::function<std::optional<dto::ItemLink>(const dto::ItemLink&, int64_t)> m_createItemLinkCallback;
    std::function<ItemLinkResult(int64_t, int64_t)> m_deleteItemLinkCallback;
    std::function<std::vector<dto::ItemLink>(int64_t, int64_t)> m_itemLinksByItemIdCallback;
    std::function<std::vector<dto::ItemLink>(int64_t, int64_t)> m_itemLinksByLinkTypeIdCallback;

    int m_getItemLinksCallCount = 0;
    int m_getItemLinkCallCount = 0;
    int m_getItemLinksByItemIdCallCount = 0;
    int m_getItemLinksByLinkTypeIdCallCount = 0;
    int m_createItemLinkCallCount = 0;
    int m_deleteItemLinkCallCount = 0;

    int m_lastGetItemLinksPage = 0;
    int m_lastGetItemLinksPageSize = 0;
    int64_t m_lastGetItemLinksUserId = 0;
    std::optional<int64_t> m_lastGetItemLinksLinkTypeId;
    std::optional<int64_t> m_lastGetItemLinksSourceItemId;
    std::optional<int64_t> m_lastGetItemLinksDestItemId;
    int64_t m_lastGetItemLinkId = 0;
    int64_t m_lastGetItemLinkUserId = 0;
    int64_t m_lastGetItemLinksByItemId = 0;
    int64_t m_lastGetItemLinksByItemIdUserId = 0;
    int64_t m_lastGetItemLinksByLinkTypeId = 0;
    int64_t m_lastGetItemLinksByLinkTypeIdUserId = 0;
    dto::ItemLink m_lastCreatedItemLink;
    int64_t m_lastCreateItemLinkUserId = 0;
    int64_t m_lastDeletedItemLinkId = 0;
    int64_t m_lastDeleteItemLinkUserId = 0;
    int64_t m_nextId = 100;
};

} // namespace server::tests
