#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_todo_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с задачами пользователя.
 */
class UserTodosHandler final : public BaseHandler
{
public:
    explicit UserTodosHandler(std::shared_ptr<services::IUserTodoService> todoService);

    /**
     * @brief Получает список задач пользователя с пагинацией и фильтрацией.
     * GET /user-todos
     */
    void handleGetTodos(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает задачу по ID.
     * GET /user-todos/{id}
     */
    void handleGetTodo(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую задачу.
     * POST /user-todos
     */
    void handleCreateTodo(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующую задачу.
     * PUT /user-todos/{id}
     */
    void handleUpdateTodo(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет задачу.
     * DELETE /user-todos/{id}
     */
    void handleDeleteTodo(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IUserTodoService> m_todoService;
};

} // namespace handlers
} // namespace server
