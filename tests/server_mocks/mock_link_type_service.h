#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "common/dto/link_type.h"
#include "logic/ilink_type_service.h"

namespace server::tests
{

class MockLinkTypeService : public services::ILinkTypeService
{
public:
    using LinkTypesPage = services::LinkTypesPage;

    void setGetLinkTypesResult(const LinkTypesPage& result)
    {
        m_getLinkTypesResult = result;
        m_getLinkTypesCallback = nullptr;
    }

    void setGetLinkTypeResult(std::optional<dto::LinkType> linkType)
    {
        m_getLinkTypeResult = std::move(linkType);
        m_getLinkTypeCallback = nullptr;
    }

    void setCreateLinkTypeResult(std::optional<dto::LinkType> linkType)
    {
        m_createLinkTypeResult = std::move(linkType);
        m_createLinkTypeCallback = nullptr;
    }

    void setUpdateLinkTypeResult(std::optional<dto::LinkType> linkType)
    {
        m_updateLinkTypeResult = std::move(linkType);
        m_updateLinkTypeCallback = nullptr;
    }

    void setDeleteLinkTypeResult(bool result)
    {
        m_deleteLinkTypeResult = result;
        m_deleteLinkTypeCallback = nullptr;
    }

    // Callback-и для кастомной логики
    void setGetLinkTypesCallback(
        std::function<LinkTypesPage(int, int, std::optional<int64_t>, std::optional<int64_t>)> callback
    )
    {
        m_getLinkTypesCallback = std::move(callback);
    }

    void setGetLinkTypeCallback(
        std::function<std::optional<dto::LinkType>(int64_t)> callback
    )
    {
        m_getLinkTypeCallback = std::move(callback);
    }

    void setCreateLinkTypeCallback(
        std::function<std::optional<dto::LinkType>(const dto::LinkType&, int64_t)> callback
    )
    {
        m_createLinkTypeCallback = std::move(callback);
    }

    void setUpdateLinkTypeCallback(
        std::function<std::optional<dto::LinkType>(const dto::LinkType&, int64_t)> callback
    )
    {
        m_updateLinkTypeCallback = std::move(callback);
    }

    void setDeleteLinkTypeCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteLinkTypeCallback = std::move(callback);
    }

    // Реализация интерфейса ILinkTypeService
    LinkTypesPage getLinkTypes(
        int page,
        int pageSize,
        std::optional<int64_t> sourceItemTypeId = std::nullopt,
        std::optional<int64_t> destinationItemTypeId = std::nullopt
    ) override
    {
        m_lastGetLinkTypesPage = page;
        m_lastGetLinkTypesPageSize = pageSize;
        m_lastGetLinkTypesSourceItemTypeId = sourceItemTypeId;
        m_lastGetLinkTypesDestItemTypeId = destinationItemTypeId;
        ++m_getLinkTypesCallCount;

        if (m_getLinkTypesCallback)
        {
            return m_getLinkTypesCallback(page, pageSize, sourceItemTypeId, destinationItemTypeId);
        }
        return m_getLinkTypesResult;
    }

    std::optional<dto::LinkType> getLinkType(int64_t id) override
    {
        m_lastGetLinkTypeId = id;
        ++m_getLinkTypeCallCount;

        if (m_getLinkTypeCallback)
        {
            return m_getLinkTypeCallback(id);
        }

        // Для обычных пользователей (не супер-админ) возвращаем nullopt,
        // чтобы симулировать отсутствие доступа или несуществующий ресурс
        // В реальном тесте это поведение настраивается через callback
        return m_getLinkTypeResult;
    }

    std::optional<dto::LinkType> createLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) override
    {
        m_lastCreatedLinkType = linkType;
        m_lastCreateLinkTypeUserId = userId;
        ++m_createLinkTypeCallCount;

        if (m_createLinkTypeCallback)
        {
            return m_createLinkTypeCallback(linkType, userId);
        }

        // ============================================================
        // ПРОВЕРКА ПРАВ В МОК-СЕРВИСЕ
        // Только супер-администратор (userId == 1) может создавать типы связей
        // ============================================================
        if (userId != 1)
        {
            // Обычный пользователь не имеет прав
            return std::nullopt;
        }
        // ============================================================

        return m_createLinkTypeResult;
    }

    std::optional<dto::LinkType> updateLinkType(
        const dto::LinkType& linkType,
        int64_t userId
    ) override
    {
        m_lastUpdatedLinkType = linkType;
        m_lastUpdateLinkTypeUserId = userId;
        ++m_updateLinkTypeCallCount;

        if (m_updateLinkTypeCallback)
        {
            return m_updateLinkTypeCallback(linkType, userId);
        }

        // ============================================================
        // ПРОВЕРКА ПРАВ В МОК-СЕРВИСЕ
        // Только супер-администратор (userId == 1) может обновлять типы связей
        // ============================================================
        if (userId != 1)
        {
            // Обычный пользователь не имеет прав
            return std::nullopt;
        }
        // ============================================================

        return m_updateLinkTypeResult;
    }

    bool deleteLinkType(int64_t id, int64_t userId) override
    {
        m_lastDeletedLinkTypeId = id;
        m_lastDeleteLinkTypeUserId = userId;
        ++m_deleteLinkTypeCallCount;

        if (m_deleteLinkTypeCallback)
        {
            return m_deleteLinkTypeCallback(id, userId);
        }

        // ============================================================
        // ПРОВЕРКА ПРАВ В МОК-СЕРВИСЕ
        // Только супер-администратор (userId == 1) может удалять типы связей
        // ============================================================
        if (userId != 1)
        {
            // Обычный пользователь не имеет прав
            return false;
        }
        // ============================================================

        return m_deleteLinkTypeResult;
    }

    // Методы для проверки вызовов
    int getGetLinkTypesCallCount() const { return m_getLinkTypesCallCount; }
    int getGetLinkTypeCallCount() const { return m_getLinkTypeCallCount; }
    int getCreateLinkTypeCallCount() const { return m_createLinkTypeCallCount; }
    int getUpdateLinkTypeCallCount() const { return m_updateLinkTypeCallCount; }
    int getDeleteLinkTypeCallCount() const { return m_deleteLinkTypeCallCount; }

    int getLastGetLinkTypesPage() const { return m_lastGetLinkTypesPage; }
    int getLastGetLinkTypesPageSize() const { return m_lastGetLinkTypesPageSize; }
    std::optional<int64_t> getLastGetLinkTypesSourceItemTypeId() const { return m_lastGetLinkTypesSourceItemTypeId; }
    std::optional<int64_t> getLastGetLinkTypesDestItemTypeId() const { return m_lastGetLinkTypesDestItemTypeId; }
    int64_t getLastGetLinkTypeId() const { return m_lastGetLinkTypeId; }
    const dto::LinkType& getLastCreatedLinkType() const { return m_lastCreatedLinkType; }
    int64_t getLastCreateLinkTypeUserId() const { return m_lastCreateLinkTypeUserId; }
    const dto::LinkType& getLastUpdatedLinkType() const { return m_lastUpdatedLinkType; }
    int64_t getLastUpdateLinkTypeUserId() const { return m_lastUpdateLinkTypeUserId; }
    int64_t getLastDeletedLinkTypeId() const { return m_lastDeletedLinkTypeId; }
    int64_t getLastDeleteLinkTypeUserId() const { return m_lastDeleteLinkTypeUserId; }

    void reset()
    {
        m_getLinkTypesCallCount = 0;
        m_getLinkTypeCallCount = 0;
        m_createLinkTypeCallCount = 0;
        m_updateLinkTypeCallCount = 0;
        m_deleteLinkTypeCallCount = 0;

        m_lastGetLinkTypesPage = 0;
        m_lastGetLinkTypesPageSize = 0;
        m_lastGetLinkTypesSourceItemTypeId.reset();
        m_lastGetLinkTypesDestItemTypeId.reset();
        m_lastGetLinkTypeId = 0;
        m_lastCreatedLinkType = dto::LinkType {};
        m_lastCreateLinkTypeUserId = 0;
        m_lastUpdatedLinkType = dto::LinkType {};
        m_lastUpdateLinkTypeUserId = 0;
        m_lastDeletedLinkTypeId = 0;
        m_lastDeleteLinkTypeUserId = 0;

        m_getLinkTypesCallback = nullptr;
        m_getLinkTypeCallback = nullptr;
        m_createLinkTypeCallback = nullptr;
        m_updateLinkTypeCallback = nullptr;
        m_deleteLinkTypeCallback = nullptr;

        m_getLinkTypesResult = LinkTypesPage {};
        m_getLinkTypeResult = std::nullopt;
        m_createLinkTypeResult = std::nullopt;
        m_updateLinkTypeResult = std::nullopt;
        m_deleteLinkTypeResult = false;
    }

private:
    LinkTypesPage m_getLinkTypesResult;
    std::optional<dto::LinkType> m_getLinkTypeResult;
    std::optional<dto::LinkType> m_createLinkTypeResult;
    std::optional<dto::LinkType> m_updateLinkTypeResult;
    bool m_deleteLinkTypeResult = false;

    std::function<LinkTypesPage(int, int, std::optional<int64_t>, std::optional<int64_t>)> m_getLinkTypesCallback;
    std::function<std::optional<dto::LinkType>(int64_t)> m_getLinkTypeCallback;
    std::function<std::optional<dto::LinkType>(const dto::LinkType&, int64_t)> m_createLinkTypeCallback;
    std::function<std::optional<dto::LinkType>(const dto::LinkType&, int64_t)> m_updateLinkTypeCallback;
    std::function<bool(int64_t, int64_t)> m_deleteLinkTypeCallback;

    int m_getLinkTypesCallCount = 0;
    int m_getLinkTypeCallCount = 0;
    int m_createLinkTypeCallCount = 0;
    int m_updateLinkTypeCallCount = 0;
    int m_deleteLinkTypeCallCount = 0;

    int m_lastGetLinkTypesPage = 0;
    int m_lastGetLinkTypesPageSize = 0;
    std::optional<int64_t> m_lastGetLinkTypesSourceItemTypeId;
    std::optional<int64_t> m_lastGetLinkTypesDestItemTypeId;
    int64_t m_lastGetLinkTypeId = 0;
    dto::LinkType m_lastCreatedLinkType;
    int64_t m_lastCreateLinkTypeUserId = 0;
    dto::LinkType m_lastUpdatedLinkType;
    int64_t m_lastUpdateLinkTypeUserId = 0;
    int64_t m_lastDeletedLinkTypeId = 0;
    int64_t m_lastDeleteLinkTypeUserId = 0;
};

} // namespace server::tests
