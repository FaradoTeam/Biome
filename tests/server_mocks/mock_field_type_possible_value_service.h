#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "common/dto/field_type_possible_value.h"

#include "logic/ifield_type_possible_value_service.h"

namespace server
{
namespace tests
{

class MockFieldTypePossibleValueService : public services::IFieldTypePossibleValueService
{
public:
    using ValuesPage = services::FieldTypePossibleValuesPage;
    using ValueResult = services::FieldTypePossibleValueResult;

    MockFieldTypePossibleValueService() = default;

    // Настройка результатов
    void setGetValuesResult(const ValuesPage& result)
    {
        m_getValuesResult = result;
        m_getValuesCallback = nullptr;
    }

    void setGetValueResult(std::optional<dto::FieldTypePossibleValue> value)
    {
        m_getValueResult = std::move(value);
        m_getValueCallback = nullptr;
    }

    void setCreateValueResult(std::optional<dto::FieldTypePossibleValue> value)
    {
        m_createValueResult = std::move(value);
        m_createValueCallback = nullptr;
    }

    void setUpdateValueResult(std::optional<dto::FieldTypePossibleValue> value)
    {
        m_updateValueResult = std::move(value);
        m_updateValueCallback = nullptr;
    }

    void setDeleteValueResult(bool result)
    {
        m_deleteValueResult = result;
        m_deleteValueCallback = nullptr;
    }

    void setValuesByFieldTypeIdResult(std::vector<dto::FieldTypePossibleValue> values)
    {
        m_valuesByFieldTypeIdResult = std::move(values);
        m_valuesByFieldTypeIdCallback = nullptr;
    }

    // Callback-и
    void setGetValuesCallback(
        std::function<ValuesPage(int, int, int64_t, std::optional<int64_t>)> callback
    )
    {
        m_getValuesCallback = std::move(callback);
    }

    void setGetValueCallback(
        std::function<std::optional<dto::FieldTypePossibleValue>(int64_t, int64_t)> callback
    )
    {
        m_getValueCallback = std::move(callback);
    }

    void setCreateValueCallback(
        std::function<std::optional<dto::FieldTypePossibleValue>(
            const dto::FieldTypePossibleValue&, int64_t
        )>
            callback
    )
    {
        m_createValueCallback = std::move(callback);
    }

    void setUpdateValueCallback(
        std::function<std::optional<dto::FieldTypePossibleValue>(
            const dto::FieldTypePossibleValue&, int64_t
        )>
            callback
    )
    {
        m_updateValueCallback = std::move(callback);
    }

    void setDeleteValueCallback(
        std::function<ValueResult(int64_t, int64_t)> callback
    )
    {
        m_deleteValueCallback = std::move(callback);
    }

    // Реализация интерфейса
    services::FieldTypePossibleValuesPage getFieldTypePossibleValues(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> fieldTypeId = std::nullopt
    ) override
    {
        m_lastGetValuesPage = page;
        m_lastGetValuesPageSize = pageSize;
        m_lastGetValuesUserId = userId;
        m_lastGetValuesFieldTypeId = fieldTypeId;
        ++m_getValuesCallCount;

        if (m_getValuesCallback)
        {
            return m_getValuesCallback(page, pageSize, userId, fieldTypeId);
        }
        return m_getValuesResult;
    }

    std::optional<dto::FieldTypePossibleValue> getFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastGetValueId = id;
        m_lastGetValueUserId = userId;
        ++m_getValueCallCount;

        if (m_getValueCallback)
        {
            return m_getValueCallback(id, userId);
        }
        return m_getValueResult;
    }

    std::vector<dto::FieldTypePossibleValue> getValuesByFieldTypeId(
        int64_t fieldTypeId,
        int64_t userId
    ) override
    {
        m_lastValuesByFieldTypeId = fieldTypeId;
        m_lastValuesByFieldTypeIdUserId = userId;
        ++m_getValuesByFieldTypeIdCallCount;

        if (m_valuesByFieldTypeIdCallback)
        {
            return m_valuesByFieldTypeIdCallback(fieldTypeId, userId);
        }
        return m_valuesByFieldTypeIdResult;
    }

    std::optional<dto::FieldTypePossibleValue> createFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) override
    {
        m_lastCreatedValue = value;
        m_lastCreateValueUserId = userId;
        ++m_createValueCallCount;

        if (m_createValueCallback)
        {
            return m_createValueCallback(value, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может создавать
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_createValueResult;
    }

    std::optional<dto::FieldTypePossibleValue> updateFieldTypePossibleValue(
        const dto::FieldTypePossibleValue& value,
        int64_t userId
    ) override
    {
        m_lastUpdatedValue = value;
        m_lastUpdateValueUserId = userId;
        ++m_updateValueCallCount;

        if (m_updateValueCallback)
        {
            return m_updateValueCallback(value, userId);
        }

        // Симуляция проверки прав: только супер-админ (userId=1) может обновлять
        if (userId != 1)
        {
            return std::nullopt;
        }
        return m_updateValueResult;
    }

    services::FieldTypePossibleValueResult deleteFieldTypePossibleValue(
        int64_t id,
        int64_t userId
    ) override
    {
        m_lastDeletedValueId = id;
        m_lastDeleteValueUserId = userId;
        ++m_deleteValueCallCount;

        if (m_deleteValueCallback)
        {
            return m_deleteValueCallback(id, userId);
        }

        services::FieldTypePossibleValueResult result;
        // Симуляция проверки прав: только супер-админ (userId=1) может удалять
        if (userId != 1)
        {
            result.success = false;
            result.errorCode = 403;
            result.errorMessage = "Insufficient permissions";
            return result;
        }

        result.success = m_deleteValueResult;
        if (!m_deleteValueResult)
        {
            result.errorCode = 404;
            result.errorMessage = "Value not found";
        }
        return result;
    }

    // Методы для проверки вызовов
    int getValuesCallCount() const { return m_getValuesCallCount; }
    int getValueCallCount() const { return m_getValueCallCount; }
    int getValuesByFieldTypeIdCallCount() const { return m_getValuesByFieldTypeIdCallCount; }
    int createValueCallCount() const { return m_createValueCallCount; }
    int updateValueCallCount() const { return m_updateValueCallCount; }
    int deleteValueCallCount() const { return m_deleteValueCallCount; }

    int getLastGetValuesPage() const { return m_lastGetValuesPage; }
    int getLastGetValuesPageSize() const { return m_lastGetValuesPageSize; }
    int64_t getLastGetValuesUserId() const { return m_lastGetValuesUserId; }
    std::optional<int64_t> getLastGetValuesFieldTypeId() const { return m_lastGetValuesFieldTypeId; }
    int64_t getLastGetValueId() const { return m_lastGetValueId; }
    int64_t getLastGetValueUserId() const { return m_lastGetValueUserId; }
    const dto::FieldTypePossibleValue& getLastCreatedValue() const { return m_lastCreatedValue; }
    int64_t getLastCreateValueUserId() const { return m_lastCreateValueUserId; }
    const dto::FieldTypePossibleValue& getLastUpdatedValue() const { return m_lastUpdatedValue; }
    int64_t getLastUpdateValueUserId() const { return m_lastUpdateValueUserId; }
    int64_t getLastDeletedValueId() const { return m_lastDeletedValueId; }
    int64_t getLastDeleteValueUserId() const { return m_lastDeleteValueUserId; }
    int64_t getLastValuesByFieldTypeId() const { return m_lastValuesByFieldTypeId; }

    void reset()
    {
        m_getValuesCallCount = 0;
        m_getValueCallCount = 0;
        m_getValuesByFieldTypeIdCallCount = 0;
        m_createValueCallCount = 0;
        m_updateValueCallCount = 0;
        m_deleteValueCallCount = 0;

        m_lastGetValuesPage = 0;
        m_lastGetValuesPageSize = 0;
        m_lastGetValuesUserId = 0;
        m_lastGetValuesFieldTypeId = std::nullopt;
        m_lastGetValueId = 0;
        m_lastGetValueUserId = 0;
        m_lastCreatedValue = dto::FieldTypePossibleValue {};
        m_lastCreateValueUserId = 0;
        m_lastUpdatedValue = dto::FieldTypePossibleValue {};
        m_lastUpdateValueUserId = 0;
        m_lastDeletedValueId = 0;
        m_lastDeleteValueUserId = 0;
        m_lastValuesByFieldTypeId = 0;

        m_getValuesCallback = nullptr;
        m_getValueCallback = nullptr;
        m_createValueCallback = nullptr;
        m_updateValueCallback = nullptr;
        m_deleteValueCallback = nullptr;
        m_valuesByFieldTypeIdCallback = nullptr;

        m_getValuesResult = ValuesPage {};
        m_getValueResult = std::nullopt;
        m_createValueResult = std::nullopt;
        m_updateValueResult = std::nullopt;
        m_deleteValueResult = false;
        m_valuesByFieldTypeIdResult.clear();
    }

private:
    ValuesPage m_getValuesResult;
    std::optional<dto::FieldTypePossibleValue> m_getValueResult;
    std::optional<dto::FieldTypePossibleValue> m_createValueResult;
    std::optional<dto::FieldTypePossibleValue> m_updateValueResult;
    bool m_deleteValueResult = false;
    std::vector<dto::FieldTypePossibleValue> m_valuesByFieldTypeIdResult;

    // Callback-и
    std::function<ValuesPage(int, int, int64_t, std::optional<int64_t>)> m_getValuesCallback;
    std::function<std::optional<dto::FieldTypePossibleValue>(int64_t, int64_t)> m_getValueCallback;
    std::function<std::optional<dto::FieldTypePossibleValue>(const dto::FieldTypePossibleValue&, int64_t)> m_createValueCallback;
    std::function<std::optional<dto::FieldTypePossibleValue>(const dto::FieldTypePossibleValue&, int64_t)> m_updateValueCallback;
    std::function<ValueResult(int64_t, int64_t)> m_deleteValueCallback;
    std::function<std::vector<dto::FieldTypePossibleValue>(int64_t, int64_t)> m_valuesByFieldTypeIdCallback;

    // Счётчики вызовов
    int m_getValuesCallCount = 0;
    int m_getValueCallCount = 0;
    int m_getValuesByFieldTypeIdCallCount = 0;
    int m_createValueCallCount = 0;
    int m_updateValueCallCount = 0;
    int m_deleteValueCallCount = 0;

    // Параметры последних вызовов
    int m_lastGetValuesPage = 0;
    int m_lastGetValuesPageSize = 0;
    int64_t m_lastGetValuesUserId = 0;
    std::optional<int64_t> m_lastGetValuesFieldTypeId;
    int64_t m_lastGetValueId = 0;
    int64_t m_lastGetValueUserId = 0;
    dto::FieldTypePossibleValue m_lastCreatedValue;
    int64_t m_lastCreateValueUserId = 0;
    dto::FieldTypePossibleValue m_lastUpdatedValue;
    int64_t m_lastUpdateValueUserId = 0;
    int64_t m_lastDeletedValueId = 0;
    int64_t m_lastDeleteValueUserId = 0;
    int64_t m_lastValuesByFieldTypeId = 0;
    int64_t m_lastValuesByFieldTypeIdUserId = 0;
};

} // namespace tests
} // namespace server
