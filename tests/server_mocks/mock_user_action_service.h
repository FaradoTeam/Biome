#pragma once

#include <optional>
#include <vector>

#include "logic/iuser_action_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-реализация сервиса действий пользователя для тестирования.
 */
class MockUserActionService : public services::IUserActionService
{
public:
    MockUserActionService() = default;

    // ============================================================
    // Настройка результатов
    // ============================================================

    void setGetActionsResult(const services::UserActionsPage& result)
    {
        m_getActionsResult = result;
    }

    void setGetActionResult(std::optional<dto::UserAction> result)
    {
        m_getActionResult = result;
    }

    void setCreateActionResult(std::optional<dto::UserAction> result)
    {
        m_createActionResult = result;
    }

    void setDeleteActionResult(const services::UserActionResult& result)
    {
        m_deleteActionResult = result;
    }

    // ============================================================
    // Получение параметров вызовов
    // ============================================================

    int getGetActionsCallCount() const { return m_getActionsCallCount; }
    int getGetActionCallCount() const { return m_getActionCallCount; }
    int getCreateActionCallCount() const { return m_createActionCallCount; }
    int getDeleteActionCallCount() const { return m_deleteActionCallCount; }

    int getLastGetActionsPage() const { return m_lastGetActionsPage; }
    int getLastGetActionsPageSize() const { return m_lastGetActionsPageSize; }
    int64_t getLastGetActionsUserId() const { return m_lastGetActionsUserId; }
    std::optional<int64_t> getLastGetActionsFilterUserId() const { return m_lastGetActionsFilterUserId; }
    std::optional<common::DateTime> getLastGetActionsDateFrom() const { return m_lastGetActionsDateFrom; }
    std::optional<common::DateTime> getLastGetActionsDateTo() const { return m_lastGetActionsDateTo; }

    int64_t getLastGetActionId() const { return m_lastGetActionId; }
    int64_t getLastGetActionUserId() const { return m_lastGetActionUserId; }

    int64_t getLastCreateActionUserId() const { return m_lastCreateActionUserId; }
    const dto::UserAction& getLastCreateAction() const { return m_lastCreateAction; }

    int64_t getLastDeleteActionId() const { return m_lastDeleteActionId; }
    int64_t getLastDeleteActionUserId() const { return m_lastDeleteActionUserId; }

    // ============================================================
    // IUserActionService implementation
    // ============================================================

    services::UserActionsPage getActions(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<common::DateTime> dateFrom = std::nullopt,
        std::optional<common::DateTime> dateTo = std::nullopt
    ) override
    {
        ++m_getActionsCallCount;
        m_lastGetActionsPage = page;
        m_lastGetActionsPageSize = pageSize;
        m_lastGetActionsUserId = userId;
        m_lastGetActionsFilterUserId = filterUserId;
        m_lastGetActionsDateFrom = dateFrom;
        m_lastGetActionsDateTo = dateTo;
        return m_getActionsResult;
    }

    std::optional<dto::UserAction> getAction(
        int64_t id,
        int64_t userId
    ) override
    {
        ++m_getActionCallCount;
        m_lastGetActionId = id;
        m_lastGetActionUserId = userId;
        return m_getActionResult;
    }

    std::optional<dto::UserAction> createAction(
        const dto::UserAction& action,
        int64_t userId
    ) override
    {
        ++m_createActionCallCount;
        m_lastCreateAction = action;
        m_lastCreateActionUserId = userId;
        return m_createActionResult;
    }

    services::UserActionResult deleteAction(
        int64_t id,
        int64_t userId
    ) override
    {
        ++m_deleteActionCallCount;
        m_lastDeleteActionId = id;
        m_lastDeleteActionUserId = userId;
        return m_deleteActionResult;
    }

    // ============================================================
    // Вспомогательные методы для создания тестовых данных
    // ============================================================

    static dto::UserAction createTestAction(
        int64_t id,
        int64_t userId,
        const std::string& caption,
        const std::string& description = ""
    )
    {
        dto::UserAction action;
        action.id = id;
        action.userId = userId;
        action.caption = caption;
        if (!description.empty())
            action.description = description;
        action.timestamp = std::chrono::system_clock::now();
        return action;
    }

    static services::UserActionsPage createTestPage(
        const std::vector<dto::UserAction>& actions,
        int64_t totalCount = -1
    )
    {
        services::UserActionsPage page;
        page.actions = actions;
        page.totalCount = (totalCount >= 0) ? totalCount : actions.size();
        return page;
    }

    static services::UserActionResult createSuccessResult()
    {
        services::UserActionResult result;
        result.success = true;
        return result;
    }

    static services::UserActionResult createErrorResult(
        int errorCode,
        const std::string& errorMessage
    )
    {
        services::UserActionResult result;
        result.success = false;
        result.errorCode = errorCode;
        result.errorMessage = errorMessage;
        return result;
    }

private:
    // Счетчики вызовов
    int m_getActionsCallCount = 0;
    int m_getActionCallCount = 0;
    int m_createActionCallCount = 0;
    int m_deleteActionCallCount = 0;

    // Результаты
    services::UserActionsPage m_getActionsResult;
    std::optional<dto::UserAction> m_getActionResult;
    std::optional<dto::UserAction> m_createActionResult;
    services::UserActionResult m_deleteActionResult;

    // Параметры вызовов
    int m_lastGetActionsPage = 0;
    int m_lastGetActionsPageSize = 0;
    int64_t m_lastGetActionsUserId = 0;
    std::optional<int64_t> m_lastGetActionsFilterUserId;
    std::optional<common::DateTime> m_lastGetActionsDateFrom;
    std::optional<common::DateTime> m_lastGetActionsDateTo;

    int64_t m_lastGetActionId = 0;
    int64_t m_lastGetActionUserId = 0;

    dto::UserAction m_lastCreateAction;
    int64_t m_lastCreateActionUserId = 0;

    int64_t m_lastDeleteActionId = 0;
    int64_t m_lastDeleteActionUserId = 0;
};

} // namespace tests
} // namespace server