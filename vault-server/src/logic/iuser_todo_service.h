#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common/dto/user_todo.h"

namespace server
{
namespace services
{

/**
 * @brief Страница с задачами пользователя.
 */
struct UserTodosPage
{
    std::vector<dto::UserTodo> todos;
    int64_t totalCount = 0;
};

/**
 * @brief Результат операции с задачей пользователя.
 */
struct UserTodoResult
{
    bool success = false;
    int errorCode = 0;
    std::string errorMessage;
};

/**
 * @brief Интерфейс сервиса для работы с задачами пользователя.
 */
class IUserTodoService
{
public:
    virtual ~IUserTodoService() = default;

    /**
     * @brief Получает список задач пользователя с пагинацией и фильтрацией.
     * @param page Номер страницы (начиная с 1)
     * @param pageSize Количество записей на странице
     * @param userId ID пользователя для проверки прав
     * @param filterUserId Фильтр по пользователю (std::nullopt - все)
     * @param isDone Фильтр по статусу выполнения (std::nullopt - все)
     * @return Страница с задачами
     */
    virtual UserTodosPage getTodos(
        int page,
        int pageSize,
        int64_t userId,
        std::optional<int64_t> filterUserId = std::nullopt,
        std::optional<bool> isDone = std::nullopt
    ) = 0;

    /**
     * @brief Получает задачу по ID.
     * @param id Идентификатор задачи
     * @param userId ID пользователя для проверки прав
     * @return DTO задачи или std::nullopt
     */
    virtual std::optional<dto::UserTodo> getTodo(
        int64_t id,
        int64_t userId
    ) = 0;

    /**
     * @brief Создаёт новую задачу.
     * @param todo DTO задачи
     * @param userId ID пользователя для проверки прав
     * @return Созданная задача или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserTodo> createTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) = 0;

    /**
     * @brief Обновляет существующую задачу.
     * @param todo DTO задачи с новыми данными
     * @param userId ID пользователя для проверки прав
     * @return Обновлённая задача или std::nullopt при ошибке
     */
    virtual std::optional<dto::UserTodo> updateTodo(
        const dto::UserTodo& todo,
        int64_t userId
    ) = 0;

    /**
     * @brief Удаляет задачу.
     * @param id Идентификатор задачи
     * @param userId ID пользователя для проверки прав
     * @return Результат операции
     */
    virtual UserTodoResult deleteTodo(
        int64_t id,
        int64_t userId
    ) = 0;
};

} // namespace services
} // namespace server
