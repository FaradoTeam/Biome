#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/dto/item_history.h"
#include "logic/iitem_history_service.h"

namespace server::tests
{

class MockItemHistoryService : public services::IItemHistoryService
{
public:
    using ItemHistoriesPage = services::ItemHistoriesPage;
    using ItemHistoryResult = services::ItemHistoryResult;

    void setGetItemHistoriesResult(const ItemHistoriesPage& result)
    {
        m_getItemHistoriesResult = result;
    }

    void setGetItemHistoryResult(std::optional<dto::ItemHistory> history)
    {
        m_getItemHistoryResult = std::move(history);
    }

    void setGetLastItemHistoryResult(std::optional<dto::ItemHistory> history)
    {
        m_getLastItemHistoryResult = std::move(history);
    }

    void setCreateItemHistoryResult(std::optional<dto::ItemHistory> history)
    {
        m_createItemHistoryResult = history;
    }

    void setCreateItemHistoryResultForUser(int64_t userId, std::optional<dto::ItemHistory> history)
    {
        m_createItemHistoryResultForUser[userId] = history;
    }

    void setDeleteItemHistoryResult(const ItemHistoryResult& result)
    {
        m_deleteItemHistoryResult = result;
    }

    // Реализация интерфейса
    ItemHistoriesPage getItemHistories(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override
    {
        m_lastGetItemHistoriesPage = page;
        m_lastGetItemHistoriesPageSize = pageSize;
        m_lastGetItemHistoriesUserId = userId;
        m_lastGetItemHistoriesItemId = itemId;
        m_lastGetItemHistoriesFilterUserId = filterUserId;
        m_lastGetItemHistoriesDateFrom = dateFrom;
        m_lastGetItemHistoriesDateTo = dateTo;
        ++m_getItemHistoriesCallCount;
        return m_getItemHistoriesResult;
    }

    std::optional<dto::ItemHistory> getItemHistory(int64_t id, int64_t userId) override
    {
        m_lastGetItemHistoryId = id;
        m_lastGetItemHistoryUserId = userId;
        ++m_getItemHistoryCallCount;
        return m_getItemHistoryResult;
    }

    std::optional<dto::ItemHistory> getLastItemHistory(int64_t itemId, int64_t userId) override
    {
        m_lastGetLastItemHistoryItemId = itemId;
        m_lastGetLastItemHistoryUserId = userId;
        ++m_getLastItemHistoryCallCount;
        return m_getLastItemHistoryResult;
    }

    std::optional<dto::ItemHistory> createItemHistory(
        const dto::ItemHistory& history,
        int64_t userId
    ) override
    {
        m_lastCreatedItemHistory = history;
        m_lastCreateItemHistoryUserId = userId;
        ++m_createItemHistoryCallCount;

        auto it = m_createItemHistoryResultForUser.find(userId);
        if (it != m_createItemHistoryResultForUser.end())
        {
            return it->second;
        }

        return m_createItemHistoryResult;
    }

    ItemHistoryResult deleteItemHistory(int64_t id, int64_t userId) override
    {
        m_lastDeletedItemHistoryId = id;
        m_lastDeleteItemHistoryUserId = userId;
        ++m_deleteItemHistoryCallCount;
        return m_deleteItemHistoryResult;
    }

    // Публичные геттеры для проверки вызовов
    int getGetItemHistoriesCallCount() const { return m_getItemHistoriesCallCount; }
    int getGetItemHistoryCallCount() const { return m_getItemHistoryCallCount; }
    int getGetLastItemHistoryCallCount() const { return m_getLastItemHistoryCallCount; }
    int getCreateItemHistoryCallCount() const { return m_createItemHistoryCallCount; }
    int getDeleteItemHistoryCallCount() const { return m_deleteItemHistoryCallCount; }

    int getLastGetItemHistoriesPage() const { return m_lastGetItemHistoriesPage; }
    int getLastGetItemHistoriesPageSize() const { return m_lastGetItemHistoriesPageSize; }
    int64_t getLastGetItemHistoriesUserId() const { return m_lastGetItemHistoriesUserId; }
    std::optional<int64_t> getLastGetItemHistoriesItemId() const { return m_lastGetItemHistoriesItemId; }
    std::optional<int64_t> getLastGetItemHistoriesFilterUserId() const { return m_lastGetItemHistoriesFilterUserId; }
    std::optional<common::DateTime> getLastGetItemHistoriesDateFrom() const { return m_lastGetItemHistoriesDateFrom; }
    std::optional<common::DateTime> getLastGetItemHistoriesDateTo() const { return m_lastGetItemHistoriesDateTo; }
    int64_t getLastGetItemHistoryId() const { return m_lastGetItemHistoryId; }
    int64_t getLastGetItemHistoryUserId() const { return m_lastGetItemHistoryUserId; }
    int64_t getLastGetLastItemHistoryItemId() const { return m_lastGetLastItemHistoryItemId; }
    int64_t getLastGetLastItemHistoryUserId() const { return m_lastGetLastItemHistoryUserId; }
    const dto::ItemHistory& getLastCreatedItemHistory() const { return m_lastCreatedItemHistory; }
    int64_t getLastCreateItemHistoryUserId() const { return m_lastCreateItemHistoryUserId; }
    int64_t getLastDeletedItemHistoryId() const { return m_lastDeletedItemHistoryId; }
    int64_t getLastDeleteItemHistoryUserId() const { return m_lastDeleteItemHistoryUserId; }

    void reset()
    {
        m_getItemHistoriesCallCount = 0;
        m_getItemHistoryCallCount = 0;
        m_getLastItemHistoryCallCount = 0;
        m_createItemHistoryCallCount = 0;
        m_deleteItemHistoryCallCount = 0;
        m_createItemHistoryResultForUser.clear();
    }

private:
    ItemHistoriesPage m_getItemHistoriesResult;
    std::optional<dto::ItemHistory> m_getItemHistoryResult;
    std::optional<dto::ItemHistory> m_getLastItemHistoryResult;
    std::optional<dto::ItemHistory> m_createItemHistoryResult;
    std::unordered_map<int64_t, std::optional<dto::ItemHistory>> m_createItemHistoryResultForUser;
    ItemHistoryResult m_deleteItemHistoryResult;

    // Счётчики вызовов
    int m_getItemHistoriesCallCount = 0;
    int m_getItemHistoryCallCount = 0;
    int m_getLastItemHistoryCallCount = 0;
    int m_createItemHistoryCallCount = 0;
    int m_deleteItemHistoryCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetItemHistoriesPage = 0;
    int m_lastGetItemHistoriesPageSize = 0;
    int64_t m_lastGetItemHistoriesUserId = 0;
    std::optional<int64_t> m_lastGetItemHistoriesItemId;
    std::optional<int64_t> m_lastGetItemHistoriesFilterUserId;
    std::optional<common::DateTime> m_lastGetItemHistoriesDateFrom;
    std::optional<common::DateTime> m_lastGetItemHistoriesDateTo;
    int64_t m_lastGetItemHistoryId = 0;
    int64_t m_lastGetItemHistoryUserId = 0;
    int64_t m_lastGetLastItemHistoryItemId = 0;
    int64_t m_lastGetLastItemHistoryUserId = 0;
    dto::ItemHistory m_lastCreatedItemHistory;
    int64_t m_lastCreateItemHistoryUserId = 0;
    int64_t m_lastDeletedItemHistoryId = 0;
    int64_t m_lastDeleteItemHistoryUserId = 0;
};

} // namespace server::tests
