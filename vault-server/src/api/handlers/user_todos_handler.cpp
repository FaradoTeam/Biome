#include <cpprest/uri.h>

#include "common/dto/user_todo.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_todos_handler.h"

namespace server
{
namespace handlers
{

UserTodosHandler::UserTodosHandler(
    std::shared_ptr<services::IUserTodoService> todoService
)
    : m_todoService(std::move(todoService))
{
    if (!m_todoService)
    {
        LOG_WARN << "UserTodosHandler инициализирован без UserTodoService";
    }
}

// ============================================================
// GET /user-todos
// ============================================================

void UserTodosHandler::handleGetTodos(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    auto params = extractQueryParams(request);

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetTodos: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetTodos: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> filterUserId = std::nullopt;
    if (params.count("userId"))
    {
        try
        {
            filterUserId = std::stoll(params["userId"]);
            if (filterUserId <= 0)
                filterUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetTodos: неверный параметр userId: " << params["userId"];
        }
    }

    std::optional<bool> isDone = std::nullopt;
    if (params.count("isDone"))
    {
        isDone = parseBool(params["isDone"]);
    }

    LOG_DEBUG
        << "GET /user-todos: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none")
        << ", isDone=" << (isDone.has_value() ? (*isDone ? "true" : "false") : "none");

    try
    {
        auto todosPage = m_todoService->getTodos(
            page, pageSize, userId, filterUserId, isDone
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < todosPage.todos.size(); ++i)
        {
            items[i] = dto::toWebJson(todosPage.todos[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(todosPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка задач: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /user-todos/{id}
// ============================================================

void UserTodosHandler::handleGetTodo(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t todoId = extractIdFromPath(request);
    if (todoId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid todo ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /user-todos/" << todoId << " from user " << userId;

    try
    {
        auto todo = m_todoService->getTodo(todoId, userId);
        if (!todo.has_value())
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Todo not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(todo->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении задачи " << todoId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /user-todos
// ============================================================

void UserTodosHandler::handleCreateTodo(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /user-todos from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::UserTodo todo(nlohmannJson);

                    // Устанавливаем пользователя, если не указан
                    if (!todo.userId.has_value())
                    {
                        todo.userId = userId;
                    }

                    // Валидация обязательных полей
                    if (!todo.caption.has_value() || todo.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Todo caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_todoService->createTodo(todo, userId);
                    if (!created.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create todo: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал задачу id=" << *created->id
                        << ", caption=" << *created->caption;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании задачи: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// PUT /user-todos/{id}
// ============================================================

void UserTodosHandler::handleUpdateTodo(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t todoId = extractIdFromPath(request);
    if (todoId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid todo ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /user-todos/" << todoId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, todoId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = todoId;
                    dto::UserTodo todo(nlohmannJson);

                    // Валидация: если передан caption, он не должен быть пустым
                    if (todo.caption.has_value() && todo.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Todo caption cannot be empty");
                        request.reply(resp);
                        return;
                    }

                    auto updated = m_todoService->updateTodo(todo, userId);
                    if (!updated.has_value())
                    {
                        // Пытаемся определить причину: нет прав или задача не найдена
                        auto existing = m_todoService->getTodo(todoId, userId);
                        if (!existing.has_value())
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Todo not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this todo");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил задачу " << todoId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении задачи " << todoId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /user-todos/{id}
// ============================================================

void UserTodosHandler::handleDeleteTodo(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t todoId = extractIdFromPath(request);
    if (todoId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid todo ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /user-todos/" << todoId << " from user " << userId;

    try
    {
        auto result = m_todoService->deleteTodo(todoId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил задачу " << todoId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении задачи " << todoId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
