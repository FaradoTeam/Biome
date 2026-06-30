#pragma once

#include <memory>

#include "logic/iauthorization_service.h"
#include "logic/iuser_todo_service.h"

#include "repo/user_todo_repository.h"

namespace server
{
namespace services
{

/**
 * @brief Реализация сервиса для работы с задачами пользователя.
 */
class UserTodoService final : public IUserTodoService
{
public:
    /**
     * @brief Конструктор.
     * @param todoRepo Репозиторий задач пользователя
     * @param authzService Сервис авторизации для проверки прав
     */
    UserTodoService(
        std::shared_ptr<repositories::IUserTodoRepository> todoRepo,
        std::shared_ptr<IAuthorizationService> authzService
    );

    // IUserTodoService
    UserTodosPage getTodos(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<bool> isDone = std::nullopt
    ) override;

    std::optional<dto::UserTodo> getTodo(
        int64_t id,
        int64_t userId
    ) override;

    std::optional<dto::UserTodo> createTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) override;

    std::optional<dto::UserTodo> updateTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) override;

    UserTodoResult deleteTodo(
        int64_t id,
        int64_t userId
    ) override;

private:
    /**
     * @brief Проверяет доступ к задаче.
     * @param todoId ID задачи
     * @param userId ID пользователя
     * @param needWrite Требуется ли право на запись
     * @return DTO задачи или std::nullopt
     */
    std::optional<dto::UserTodo> checkTodoAccess(
        int64_t todoId,
        int64_t userId,
        bool needWrite = false
    );

    /**
     * @brief Валидирует DTO задачи.
     * @param todo DTO для проверки
     * @param errorMessage Сообщение об ошибке
     * @return true если DTO валиден
     */
    bool validateTodo(
        const dto::UserTodo& todo,
        std::string& errorMessage
    );

private:
    std::shared_ptr<repositories::IUserTodoRepository> m_todoRepo;
    std::shared_ptr<IAuthorizationService> m_authzService;
};

} // namespace services
} // namespace server
