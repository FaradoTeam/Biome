#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/dto/item_user_state.h"
#include "logic/iitem_user_state_service.h"

namespace server::tests
{

class MockItemUserStateService : public services::IItemUserStateService
{
public:
    using ItemUserStatesPage = services::ItemUserStatesPage;
    using ItemUserStateResult = services::ItemUserStateResult;

    void setGetItemUserStatesResult(const ItemUserStatesPage& result)
    {
        m_getItemUserStatesResult = result;
    }

    void setGetItemUserStateResult(std::optional<dto::ItemUserState> state)
    {
        m_getItemUserStateResult = std::move(state);
    }

    void setGetLastItemUserStateResult(std::optional<dto::ItemUserState> state)
    {
        m_getLastItemUserStateResult = std::move(state);
    }

    void setCreateItemUserStateResult(std::optional<dto::ItemUserState> state)
    {
        m_createItemUserStateResult = state;
    }

    void setCreateItemUserStateResultForUser(int64_t userId, std::optional<dto::ItemUserState> state)
    {
        m_createItemUserStateResultForUser[userId] = state;
    }

    void setDeleteItemUserStateResult(const ItemUserStateResult& result)
    {
        m_deleteItemUserStateResult = result;
    }

    // Реализация интерфейса
    ItemUserStatesPage getItemUserStates(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> itemId = std::nullopt,
        std::optional<int64_t> filterUserId = std::nullopt
    ) override
    {
        m_lastGetItemUserStatesPage = page;
        m_lastGetItemUserStatesPageSize = pageSize;
        m_lastGetItemUserStatesUserId = userId;
        m_lastGetItemUserStatesItemId = itemId;
        m_lastGetItemUserStatesFilterUserId = filterUserId;
        ++m_getItemUserStatesCallCount;
        return m_getItemUserStatesResult;
    }

    std::optional<dto::ItemUserState> getItemUserState(int64_t id, int64_t userId) override
    {
        m_lastGetItemUserStateId = id;
        m_lastGetItemUserStateUserId = userId;
        ++m_getItemUserStateCallCount;
        return m_getItemUserStateResult;
    }

    std::optional<dto::ItemUserState> getLastItemUserState(int64_t itemId, int64_t userId) override
    {
        m_lastGetLastItemUserStateItemId = itemId;
        m_lastGetLastItemUserStateUserId = userId;
        ++m_getLastItemUserStateCallCount;
        return m_getLastItemUserStateResult;
    }

    std::optional<dto::ItemUserState> createItemUserState(
        const dto::ItemUserState& state,
        int64_t userId
    ) override
    {
        m_lastCreatedItemUserState = state;
        m_lastCreateItemUserStateUserId = userId;
        ++m_createItemUserStateCallCount;

        auto it = m_createItemUserStateResultForUser.find(userId);
        if (it != m_createItemUserStateResultForUser.end())
        {
            return it->second;
        }

        return m_createItemUserStateResult;
    }

    ItemUserStateResult deleteItemUserState(int64_t id, int64_t userId) override
    {
        m_lastDeletedItemUserStateId = id;
        m_lastDeleteItemUserStateUserId = userId;
        ++m_deleteItemUserStateCallCount;
        return m_deleteItemUserStateResult;
    }

    // Публичные геттеры для проверки вызовов
    int getGetItemUserStatesCallCount() const { return m_getItemUserStatesCallCount; }
    int getGetItemUserStateCallCount() const { return m_getItemUserStateCallCount; }
    int getGetLastItemUserStateCallCount() const { return m_getLastItemUserStateCallCount; }
    int getCreateItemUserStateCallCount() const { return m_createItemUserStateCallCount; }
    int getDeleteItemUserStateCallCount() const { return m_deleteItemUserStateCallCount; }

    int getLastGetItemUserStatesPage() const { return m_lastGetItemUserStatesPage; }
    int getLastGetItemUserStatesPageSize() const { return m_lastGetItemUserStatesPageSize; }
    int64_t getLastGetItemUserStatesUserId() const { return m_lastGetItemUserStatesUserId; }
    std::optional<int64_t> getLastGetItemUserStatesItemId() const { return m_lastGetItemUserStatesItemId; }
    std::optional<int64_t> getLastGetItemUserStatesFilterUserId() const { return m_lastGetItemUserStatesFilterUserId; }
    int64_t getLastGetItemUserStateId() const { return m_lastGetItemUserStateId; }
    int64_t getLastGetItemUserStateUserId() const { return m_lastGetItemUserStateUserId; }
    int64_t getLastGetLastItemUserStateItemId() const { return m_lastGetLastItemUserStateItemId; }
    int64_t getLastGetLastItemUserStateUserId() const { return m_lastGetLastItemUserStateUserId; }
    const dto::ItemUserState& getLastCreatedItemUserState() const { return m_lastCreatedItemUserState; }
    int64_t getLastCreateItemUserStateUserId() const { return m_lastCreateItemUserStateUserId; }
    int64_t getLastDeletedItemUserStateId() const { return m_lastDeletedItemUserStateId; }
    int64_t getLastDeleteItemUserStateUserId() const { return m_lastDeleteItemUserStateUserId; }

    void reset()
    {
        m_getItemUserStatesCallCount = 0;
        m_getItemUserStateCallCount = 0;
        m_getLastItemUserStateCallCount = 0;
        m_createItemUserStateCallCount = 0;
        m_deleteItemUserStateCallCount = 0;
        m_createItemUserStateResultForUser.clear();
    }

private:
    ItemUserStatesPage m_getItemUserStatesResult;
    std::optional<dto::ItemUserState> m_getItemUserStateResult;
    std::optional<dto::ItemUserState> m_getLastItemUserStateResult;
    std::optional<dto::ItemUserState> m_createItemUserStateResult;
    std::unordered_map<int64_t, std::optional<dto::ItemUserState>> m_createItemUserStateResultForUser;
    ItemUserStateResult m_deleteItemUserStateResult;

    // Счётчики вызовов
    int m_getItemUserStatesCallCount = 0;
    int m_getItemUserStateCallCount = 0;
    int m_getLastItemUserStateCallCount = 0;
    int m_createItemUserStateCallCount = 0;
    int m_deleteItemUserStateCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetItemUserStatesPage = 0;
    int m_lastGetItemUserStatesPageSize = 0;
    int64_t m_lastGetItemUserStatesUserId = 0;
    std::optional<int64_t> m_lastGetItemUserStatesItemId;
    std::optional<int64_t> m_lastGetItemUserStatesFilterUserId;
    int64_t m_lastGetItemUserStateId = 0;
    int64_t m_lastGetItemUserStateUserId = 0;
    int64_t m_lastGetLastItemUserStateItemId = 0;
    int64_t m_lastGetLastItemUserStateUserId = 0;
    dto::ItemUserState m_lastCreatedItemUserState;
    int64_t m_lastCreateItemUserStateUserId = 0;
    int64_t m_lastDeletedItemUserStateId = 0;
    int64_t m_lastDeleteItemUserStateUserId = 0;
};

} // namespace server::tests
