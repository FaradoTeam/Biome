#include "common/log/log.h"

#include "user_todo_service.h"

namespace server
{
namespace services
{

UserTodoService::UserTodoService(
    std::shared_ptr<repositories::IUserTodoRepository> todoRepo,
    std::shared_ptr<IAuthorizationService> authzService
)
    : m_todoRepo(std::move(todoRepo))
    , m_authzService(std::move(authzService))
{
    if (!m_todoRepo)
    {
        throw std::runtime_error("UserTodoService: репозиторий задач не инициализирован");
    }
    if (!m_authzService)
    {
        throw std::runtime_error("UserTodoService: сервис авторизации не инициализирован");
    }
}

UserTodosPage UserTodoService::getTodos(
    int page,
    int pageSize,
    int64_t userId,
    std::optional<int64_t> filterUserId,
    std::optional<bool> isDone
)
{
    if (page < 1)
        page = 1;
    if (pageSize < 1)
        pageSize = 20;
    if (pageSize > 100)
        pageSize = 100;

    repositories::UserTodosPage repoPage;

    // Супер-админ может видеть задачи всех пользователей
    if (m_authzService->isSuperAdmin(userId))
    {
        repoPage = m_todoRepo->findAll(page, pageSize, filterUserId, isDone);
    }
    // Обычный пользователь может видеть только свои задачи
    else if (filterUserId.has_value() && *filterUserId != userId)
    {
        LOG_WARN
            << "getTodos: пользователь " << userId
            << " пытается получить задачи другого пользователя " << *filterUserId;
        return { {}, 0 };
    }
    else
    {
        repoPage = m_todoRepo->findAll(page, pageSize, userId, isDone);
    }

    // Конвертируем из репозиторной структуры в сервисную
    UserTodosPage result;
    result.todos = std::move(repoPage.todos);
    result.totalCount = repoPage.totalCount;
    return result;
}

std::optional<dto::UserTodo> UserTodoService::getTodo(
    int64_t id,
    int64_t userId
)
{
    return checkTodoAccess(id, userId, false);
}

std::optional<dto::UserTodo> UserTodoService::createTodo(
    const dto::UserTodo& todo,
    int64_t userId
)
{
    // 1. Валидация
    std::string errorMessage;
    if (!validateTodo(todo, errorMessage))
    {
        LOG_WARN << "createTodo: " << errorMessage;
        return std::nullopt;
    }

    // 2. Проверяем, что пользователь создаёт задачу для себя
    if (!todo.userId.has_value() || *todo.userId != userId)
    {
        // Только супер-админ может создавать задачи для других пользователей
        if (!m_authzService->isSuperAdmin(userId))
        {
            LOG_WARN
                << "createTodo: пользователь " << userId
                << " пытается создать задачу для другого пользователя " << *todo.userId;
            return std::nullopt;
        }
    }

    // 3. Создаём задачу
    const int64_t newId = m_todoRepo->create(todo);
    if (newId <= 0)
    {
        LOG_ERROR << "createTodo: не удалось создать задачу";
        return std::nullopt;
    }

    LOG_INFO
        << "Задача создана: id=" << newId
        << ", пользователь=" << userId
        << ", caption=" << *todo.caption;

    return m_todoRepo->findById(newId);
}

std::optional<dto::UserTodo> UserTodoService::updateTodo(
    const dto::UserTodo& todo,
    int64_t userId
)
{
    if (!todo.id.has_value())
    {
        LOG_WARN << "updateTodo: отсутствует ID задачи";
        return std::nullopt;
    }

    // 1. Проверяем существование и доступ к задаче
    auto existing = checkTodoAccess(*todo.id, userId, true);
    if (!existing.has_value())
    {
        return std::nullopt;
    }

    // 2. Валидируем обновлённые данные (если переданы)
    if (todo.caption.has_value())
    {
        std::string errorMessage;
        if (!validateTodo(todo, errorMessage))
        {
            LOG_WARN << "updateTodo: " << errorMessage;
            return std::nullopt;
        }
    }

    // 3. Обновляем задачу
    if (!m_todoRepo->update(todo))
    {
        LOG_ERROR
            << "updateTodo: не удалось обновить задачу id=" << *todo.id;
        return std::nullopt;
    }

    LOG_INFO
        << "Задача обновлена: id=" << *todo.id
        << ", пользователь=" << userId;

    return m_todoRepo->findById(*todo.id);
}

UserTodoResult UserTodoService::deleteTodo(
    int64_t id,
    int64_t userId
)
{
    UserTodoResult result;

    // 1. Проверяем существование и доступ к задаче
    auto existing = checkTodoAccess(id, userId, true);
    if (!existing.has_value())
    {
        result.errorMessage = "Задача не найдена или нет доступа";
        result.errorCode = 404;
        return result;
    }

    // 2. Удаляем задачу
    if (!m_todoRepo->remove(id))
    {
        result.errorMessage = "Не удалось удалить задачу";
        result.errorCode = 500;
        return result;
    }

    result.success = true;
    LOG_INFO
        << "Задача удалена: id=" << id
        << ", пользователь=" << userId;

    return result;
}

std::optional<dto::UserTodo> UserTodoService::checkTodoAccess(
    int64_t todoId,
    int64_t userId,
    bool needWrite
)
{
    auto todo = m_todoRepo->findById(todoId);
    if (!todo.has_value())
    {
        LOG_DEBUG
            << "checkTodoAccess: задача не найдена, id=" << todoId;
        return std::nullopt;
    }

    // Супер-админ имеет полный доступ
    if (m_authzService->isSuperAdmin(userId))
    {
        return todo;
    }

    // Обычный пользователь может видеть только свои задачи
    if (!todo->userId.has_value() || *todo->userId != userId)
    {
        LOG_WARN
            << "checkTodoAccess: пользователь " << userId
            << " не является владельцем задачи " << todoId;
        return std::nullopt;
    }

    return todo;
}

bool UserTodoService::validateTodo(
    const dto::UserTodo& todo,
    std::string& errorMessage
)
{
    if (!todo.userId.has_value())
    {
        errorMessage = "userId является обязательным полем";
        return false;
    }

    if (!todo.caption.has_value() || todo.caption->empty())
    {
        errorMessage = "caption является обязательным полем и не может быть пустым";
        return false;
    }

    if (todo.caption->length() > 255)
    {
        errorMessage = "caption не может превышать 255 символов";
        return false;
    }

    // isDone не обязателен, по умолчанию false

    return true;
}

} // namespace services
} // namespace server
