#pragma once

#include <optional>
#include <vector>

#include "logic/iuser_todo_service.h"

namespace server
{
namespace tests
{

/**
 * @brief Mock-реализация сервиса задач пользователя для тестирования.
 */
class MockUserTodoService : public services::IUserTodoService
{
public:
    MockUserTodoService() = default;

    // ============================================================
    // Настройка результатов
    // ============================================================

    void setGetTodosResult(const services::UserTodosPage& result)
    {
        m_getTodosResult = result;
    }

    void setGetTodoResult(std::optional<dto::UserTodo> result)
    {
        m_getTodoResult = result;
    }

    void setCreateTodoResult(std::optional<dto::UserTodo> result)
    {
        m_createTodoResult = result;
    }

    void setUpdateTodoResult(std::optional<dto::UserTodo> result)
    {
        m_updateTodoResult = result;
    }

    void setDeleteTodoResult(const services::UserTodoResult& result)
    {
        m_deleteTodoResult = result;
    }

    // ============================================================
    // Получение параметров вызовов
    // ============================================================

    int getGetTodosCallCount() const { return m_getTodosCallCount; }
    int getGetTodoCallCount() const { return m_getTodoCallCount; }
    int getCreateTodoCallCount() const { return m_createTodoCallCount; }
    int getUpdateTodoCallCount() const { return m_updateTodoCallCount; }
    int getDeleteTodoCallCount() const { return m_deleteTodoCallCount; }

    int getLastGetTodosPage() const { return m_lastGetTodosPage; }
    int getLastGetTodosPageSize() const { return m_lastGetTodosPageSize; }
    int64_t getLastGetTodosUserId() const { return m_lastGetTodosUserId; }
    std::optional<int64_t> getLastGetTodosFilterUserId() const { return m_lastGetTodosFilterUserId; }
    std::optional<bool> getLastGetTodosIsDone() const { return m_lastGetTodosIsDone; }

    int64_t getLastGetTodoId() const { return m_lastGetTodoId; }
    int64_t getLastGetTodoUserId() const { return m_lastGetTodoUserId; }

    int64_t getLastCreateTodoUserId() const { return m_lastCreateTodoUserId; }
    const dto::UserTodo& getLastCreateTodo() const { return m_lastCreateTodo; }

    int64_t getLastUpdateTodoId() const { return m_lastUpdateTodoId; }
    int64_t getLastUpdateTodoUserId() const { return m_lastUpdateTodoUserId; }
    const dto::UserTodo& getLastUpdateTodo() const { return m_lastUpdateTodo; }

    int64_t getLastDeleteTodoId() const { return m_lastDeleteTodoId; }
    int64_t getLastDeleteTodoUserId() const { return m_lastDeleteTodoUserId; }

    // ============================================================
    // IUserTodoService implementation
    // ============================================================

    services::UserTodosPage getTodos(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<bool> isDone = std::nullopt
    ) override
    {
        ++m_getTodosCallCount;
        m_lastGetTodosPage = page;
        m_lastGetTodosPageSize = pageSize;
        m_lastGetTodosUserId = userId;
        m_lastGetTodosFilterUserId = filterUserId;
        m_lastGetTodosIsDone = isDone;
        return m_getTodosResult;
    }

    std::optional<dto::UserTodo> getTodo(
        int64_t id,
        int64_t userId
    ) override
    {
        ++m_getTodoCallCount;
        m_lastGetTodoId = id;
        m_lastGetTodoUserId = userId;
        return m_getTodoResult;
    }

    std::optional<dto::UserTodo> createTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) override
    {
        ++m_createTodoCallCount;
        m_lastCreateTodo = todo;
        m_lastCreateTodoUserId = userId;
        return m_createTodoResult;
    }

    std::optional<dto::UserTodo> updateTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) override
    {
        ++m_updateTodoCallCount;
        m_lastUpdateTodo = todo;
        m_lastUpdateTodoId = todo.id.value_or(0);
        m_lastUpdateTodoUserId = userId;
        return m_updateTodoResult;
    }

    services::UserTodoResult deleteTodo(
        int64_t id,
        int64_t userId
    ) override
    {
        ++m_deleteTodoCallCount;
        m_lastDeleteTodoId = id;
        m_lastDeleteTodoUserId = userId;
        return m_deleteTodoResult;
    }

    // ============================================================
    // Вспомогательные методы для создания тестовых данных
    // ============================================================

    static dto::UserTodo createTestTodo(
        int64_t id,
        int64_t userId,
        const std::string& caption,
        bool isDone = false
    )
    {
        dto::UserTodo todo;
        todo.id = id;
        todo.userId = userId;
        todo.caption = caption;
        todo.isDone = isDone;
        return todo;
    }

    static services::UserTodosPage createTestPage(
        const std::vector<dto::UserTodo>& todos,
        int64_t totalCount = -1
    )
    {
        services::UserTodosPage page;
        page.todos = todos;
        page.totalCount = (totalCount >= 0) ? totalCount : todos.size();
        return page;
    }

    static services::UserTodoResult createSuccessResult()
    {
        services::UserTodoResult result;
        result.success = true;
        return result;
    }

    static services::UserTodoResult createErrorResult(
        int errorCode,
        const std::string& errorMessage
    )
    {
        services::UserTodoResult result;
        result.success = false;
        result.errorCode = errorCode;
        result.errorMessage = errorMessage;
        return result;
    }

private:
    // Счетчики вызовов
    int m_getTodosCallCount = 0;
    int m_getTodoCallCount = 0;
    int m_createTodoCallCount = 0;
    int m_updateTodoCallCount = 0;
    int m_deleteTodoCallCount = 0;

    // Результаты
    services::UserTodosPage m_getTodosResult;
    std::optional<dto::UserTodo> m_getTodoResult;
    std::optional<dto::UserTodo> m_createTodoResult;
    std::optional<dto::UserTodo> m_updateTodoResult;
    services::UserTodoResult m_deleteTodoResult;

    // Параметры вызовов
    int m_lastGetTodosPage = 0;
    int m_lastGetTodosPageSize = 0;
    int64_t m_lastGetTodosUserId = 0;
    std::optional<int64_t> m_lastGetTodosFilterUserId;
    std::optional<bool> m_lastGetTodosIsDone;

    int64_t m_lastGetTodoId = 0;
    int64_t m_lastGetTodoUserId = 0;

    dto::UserTodo m_lastCreateTodo;
    int64_t m_lastCreateTodoUserId = 0;

    dto::UserTodo m_lastUpdateTodo;
    int64_t m_lastUpdateTodoId = 0;
    int64_t m_lastUpdateTodoUserId = 0;

    int64_t m_lastDeleteTodoId = 0;
    int64_t m_lastDeleteTodoUserId = 0;
};

} // namespace tests
} // namespace server
