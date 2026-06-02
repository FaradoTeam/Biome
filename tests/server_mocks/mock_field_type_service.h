#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/dto/field_type.h"

#include "logic/ifield_type_service.h"

namespace server
{
namespace tests
{

class MockFieldTypeService : public services::IFieldTypeService
{
public:
    using FieldTypesPage = services::FieldTypesPage;

    void setGetFieldTypesResult(const FieldTypesPage& result)
    {
        m_getFieldTypesResult = result;
        m_getFieldTypesCallback = nullptr;
    }

    void setGetFieldTypeResult(std::optional<dto::FieldType> fieldType)
    {
        m_getFieldTypeResult = std::move(fieldType);
        m_getFieldTypeCallback = nullptr;
    }

    void setCreateFieldTypeResult(std::optional<dto::FieldType> fieldType)
    {
        m_createFieldTypeResult = std::move(fieldType);
        m_createFieldTypeCallback = nullptr;
    }

    void setUpdateFieldTypeResult(std::optional<dto::FieldType> fieldType)
    {
        m_updateFieldTypeResult = std::move(fieldType);
        m_updateFieldTypeCallback = nullptr;
    }

    void setDeleteFieldTypeResult(bool result)
    {
        m_deleteFieldTypeResult = result;
        m_deleteFieldTypeCallback = nullptr;
    }

    void setFieldTypesByItemTypeResult(std::vector<dto::FieldType> fieldTypes)
    {
        m_fieldTypesByItemTypeResult = std::move(fieldTypes);
    }

    // Callback-и
    void setGetFieldTypesCallback(
        std::function<FieldTypesPage(int, int, std::optional<int64_t>, std::optional<std::string>, const std::string&)> callback
    )
    {
        m_getFieldTypesCallback = std::move(callback);
    }

    void setGetFieldTypeCallback(
        std::function<std::optional<dto::FieldType>(int64_t)> callback
    )
    {
        m_getFieldTypeCallback = std::move(callback);
    }

    void setCreateFieldTypeCallback(
        std::function<std::optional<dto::FieldType>(const dto::FieldType&, int64_t)> callback
    )
    {
        m_createFieldTypeCallback = std::move(callback);
    }

    void setUpdateFieldTypeCallback(
        std::function<std::optional<dto::FieldType>(const dto::FieldType&, int64_t)> callback
    )
    {
        m_updateFieldTypeCallback = std::move(callback);
    }

    void setDeleteFieldTypeCallback(
        std::function<bool(int64_t, int64_t)> callback
    )
    {
        m_deleteFieldTypeCallback = std::move(callback);
    }

    // Реализация интерфейса
    FieldTypesPage fieldTypes(
        int page,
        int pageSize,
        std::optional<int64_t> itemTypeId,
        std::optional<std::string> valueType,
        const std::string& searchCaption
    ) override
    {
        m_lastGetFieldTypesPage = page;
        m_lastGetFieldTypesPageSize = pageSize;
        m_lastGetFieldTypesItemTypeId = itemTypeId;
        m_lastGetFieldTypesValueType = valueType;
        m_lastGetFieldTypesSearch = searchCaption;
        m_getFieldTypesCallCount++;

        if (m_getFieldTypesCallback)
        {
            return m_getFieldTypesCallback(page, pageSize, itemTypeId, valueType, searchCaption);
        }
        return m_getFieldTypesResult;
    }

    std::optional<dto::FieldType> fieldType(int64_t id) override
    {
        m_lastGetFieldTypeId = id;
        m_getFieldTypeCallCount++;

        if (m_getFieldTypeCallback)
        {
            return m_getFieldTypeCallback(id);
        }
        return m_getFieldTypeResult;
    }

    std::optional<dto::FieldType> createFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) override
    {
        m_lastCreatedFieldType = fieldType;
        m_lastCreateFieldTypeUserId = userId;
        m_createFieldTypeCallCount++;

        if (m_createFieldTypeCallback)
        {
            return m_createFieldTypeCallback(fieldType, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createFieldTypeResult;
    }

    std::optional<dto::FieldType> updateFieldType(
        const dto::FieldType& fieldType,
        int64_t userId
    ) override
    {
        m_lastUpdatedFieldType = fieldType;
        m_lastUpdateFieldTypeUserId = userId;
        m_updateFieldTypeCallCount++;

        if (m_updateFieldTypeCallback)
        {
            return m_updateFieldTypeCallback(fieldType, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateFieldTypeResult;
    }

    bool deleteFieldType(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedFieldTypeId = id;
        m_lastDeleteFieldTypeUserId = userId;
        m_deleteFieldTypeCallCount++;

        if (m_deleteFieldTypeCallback)
        {
            return m_deleteFieldTypeCallback(id, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            return false;
        }
        return m_deleteFieldTypeResult;
    }

    std::vector<dto::FieldType> fieldTypesByItemType(int64_t itemTypeId) override
    {
        m_lastFieldTypesByItemTypeId = itemTypeId;
        m_fieldTypesByItemTypeCallCount++;
        return m_fieldTypesByItemTypeResult;
    }

    // Методы для проверки вызовов
    int getFieldTypesCallCount() const { return m_getFieldTypesCallCount; }
    int getFieldTypeCallCount() const { return m_getFieldTypeCallCount; }
    int createFieldTypeCallCount() const { return m_createFieldTypeCallCount; }
    int updateFieldTypeCallCount() const { return m_updateFieldTypeCallCount; }
    int deleteFieldTypeCallCount() const { return m_deleteFieldTypeCallCount; }

    int getLastGetFieldTypesPage() const { return m_lastGetFieldTypesPage; }
    int64_t getLastGetFieldTypeId() const { return m_lastGetFieldTypeId; }
    const dto::FieldType& getLastCreatedFieldType() const { return m_lastCreatedFieldType; }
    int64_t getLastCreateFieldTypeUserId() const { return m_lastCreateFieldTypeUserId; }
    const dto::FieldType& getLastUpdatedFieldType() const { return m_lastUpdatedFieldType; }
    int64_t getLastUpdateFieldTypeUserId() const { return m_lastUpdateFieldTypeUserId; }
    int64_t getLastDeletedFieldTypeId() const { return m_lastDeletedFieldTypeId; }
    int64_t getLastDeleteFieldTypeUserId() const { return m_lastDeleteFieldTypeUserId; }

    void reset()
    {
        m_getFieldTypesCallCount = 0;
        m_getFieldTypeCallCount = 0;
        m_createFieldTypeCallCount = 0;
        m_updateFieldTypeCallCount = 0;
        m_deleteFieldTypeCallCount = 0;
        m_fieldTypesByItemTypeCallCount = 0;

        m_lastGetFieldTypesPage = 0;
        m_lastGetFieldTypesPageSize = 0;
        m_lastGetFieldTypeId = 0;
        m_lastDeletedFieldTypeId = 0;
        m_lastCreateFieldTypeUserId = 0;
        m_lastUpdateFieldTypeUserId = 0;
        m_lastDeleteFieldTypeUserId = 0;
    }

private:
    FieldTypesPage m_getFieldTypesResult;
    std::optional<dto::FieldType> m_getFieldTypeResult;
    std::optional<dto::FieldType> m_createFieldTypeResult;
    std::optional<dto::FieldType> m_updateFieldTypeResult;
    bool m_deleteFieldTypeResult = false;
    std::vector<dto::FieldType> m_fieldTypesByItemTypeResult;

    // Callback-и
    std::function<FieldTypesPage(int, int, std::optional<int64_t>, std::optional<std::string>, const std::string&)> m_getFieldTypesCallback;
    std::function<std::optional<dto::FieldType>(int64_t)> m_getFieldTypeCallback;
    std::function<std::optional<dto::FieldType>(const dto::FieldType&, int64_t)> m_createFieldTypeCallback;
    std::function<std::optional<dto::FieldType>(const dto::FieldType&, int64_t)> m_updateFieldTypeCallback;
    std::function<bool(int64_t, int64_t)> m_deleteFieldTypeCallback;

    int m_getFieldTypesCallCount = 0;
    int m_getFieldTypeCallCount = 0;
    int m_createFieldTypeCallCount = 0;
    int m_updateFieldTypeCallCount = 0;
    int m_deleteFieldTypeCallCount = 0;
    int m_fieldTypesByItemTypeCallCount = 0;

    int m_lastGetFieldTypesPage = 0;
    int m_lastGetFieldTypesPageSize = 0;
    std::optional<int64_t> m_lastGetFieldTypesItemTypeId;
    std::optional<std::string> m_lastGetFieldTypesValueType;
    std::string m_lastGetFieldTypesSearch;
    int64_t m_lastGetFieldTypeId = 0;
    dto::FieldType m_lastCreatedFieldType;
    int64_t m_lastCreateFieldTypeUserId = 0;
    dto::FieldType m_lastUpdatedFieldType;
    int64_t m_lastUpdateFieldTypeUserId = 0;
    int64_t m_lastDeletedFieldTypeId = 0;
    int64_t m_lastDeleteFieldTypeUserId = 0;
    int64_t m_lastFieldTypesByItemTypeId = 0;
};

} // namespace tests
} // namespace server
