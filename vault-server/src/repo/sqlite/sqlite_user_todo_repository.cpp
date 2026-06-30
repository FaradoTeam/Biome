#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "common/helpers/string_helper.h"
#include "common/log/log.h"

#include "storage/idatabase.h"

#include "sqlite_user_todo_repository.h"

namespace server
{
namespace repositories
{

SqliteUserTodoRepository::SqliteUserTodoRepository(
    std::shared_ptr<db::IDatabase> database
)
    : m_database(std::move(database))
{
    if (!m_database)
    {
        throw std::runtime_error("SqliteUserTodoRepository: database is null");
    }
}

std::shared_ptr<db::IConnection> SqliteUserTodoRepository::connection() const
{
    return m_database->connection();
}

dto::UserTodo SqliteUserTodoRepository::mapRowToUserTodo(db::IResultSet& rs) const
{
    dto::UserTodo todo;
    todo.id = rs.valueInt64("id");
    todo.userId = rs.valueInt64("userId");
    todo.isDone = rs.valueInt64("isDone") != 0;
    todo.caption = rs.valueString("caption");

    return todo;
}

UserTodosPage SqliteUserTodoRepository::findAll(
    int page,
    int pageSize,
    std::optional<int64_t> userId,
    std::optional<bool> isDone
)
{
    std::vector<dto::UserTodo> todos;
    int64_t totalCount = 0;

    try
    {
        auto conn = connection();

        // Формируем условия фильтрации
        std::vector<std::string> whereClauses;
        if (userId.has_value())
            whereClauses.push_back("userId = :userId");
        if (isDone.has_value())
            whereClauses.push_back("isDone = :isDone");

        std::string whereClause;
        if (!whereClauses.empty())
        {
            whereClause = " WHERE " + boost::algorithm::join(whereClauses, " AND ");
        }

        // Получаем общее количество
        auto countStmt = conn->prepareStatement(
            "SELECT COUNT(*) FROM UserTodo" + whereClause
        );

        if (userId.has_value())
            countStmt->bindInt64("userId", *userId);
        if (isDone.has_value())
            countStmt->bindInt64("isDone", *isDone ? 1 : 0);

        auto countRs = countStmt->executeQuery();
        if (countRs->next())
        {
            totalCount = countRs->valueInt64(0);
        }

        if (totalCount == 0 || (page - 1) * pageSize >= totalCount)
        {
            return { todos, totalCount };
        }

        // Получаем страницу с задачами
        const int offset = (page - 1) * pageSize;
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, isDone, caption, searchCaption "
            "FROM UserTodo"
            + whereClause + " ORDER BY id LIMIT :limit OFFSET :offset"
        );

        if (userId.has_value())
            stmt->bindInt64("userId", *userId);
        if (isDone.has_value())
            stmt->bindInt64("isDone", *isDone ? 1 : 0);

        stmt->bindInt64("limit", pageSize);
        stmt->bindInt64("offset", offset);

        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            todos.push_back(mapRowToUserTodo(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения списка задач пользователя: " << e.what();
        throw;
    }

    return { todos, totalCount };
}

std::optional<dto::UserTodo> SqliteUserTodoRepository::findById(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "findById: неверный идентификатор " << id;
        return std::nullopt;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, isDone, caption, searchCaption "
            "FROM UserTodo WHERE id = :id"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();

        if (rs->next())
        {
            return mapRowToUserTodo(*rs);
        }

        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка поиска задачи по id: " << e.what();
        throw;
    }
}

std::vector<dto::UserTodo> SqliteUserTodoRepository::findByUserId(int64_t userId)
{
    std::vector<dto::UserTodo> todos;

    if (userId <= 0)
    {
        LOG_WARN << "findByUserId: неверный userId " << userId;
        return todos;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT id, userId, isDone, caption, searchCaption "
            "FROM UserTodo WHERE userId = :userId ORDER BY id"
        );

        stmt->bindInt64("userId", userId);
        auto rs = stmt->executeQuery();

        while (rs->next())
        {
            todos.push_back(mapRowToUserTodo(*rs));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка получения задач пользователя: " << e.what();
        throw;
    }

    return todos;
}

int64_t SqliteUserTodoRepository::create(const dto::UserTodo& todo)
{
    if (!todo.userId.has_value() || !todo.caption.has_value() || todo.caption->empty())
    {
        LOG_WARN << "create: отсутствуют обязательные поля";
        return 0;
    }

    try
    {
        auto conn = connection();

        auto stmt = conn->prepareStatement(
            "INSERT INTO UserTodo (userId, isDone, caption, searchCaption) "
            "VALUES (:userId, :isDone, :caption, :searchCaption)"
        );

        stmt->bindInt64("userId", *todo.userId);
        stmt->bindInt64("isDone", todo.isDone.value_or(false) ? 1 : 0);
        stmt->bindString("caption", *todo.caption);
        stmt->bindString("searchCaption", common::toLowerCase(*todo.caption));

        stmt->execute();
        return conn->lastInsertId();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка создания задачи: " << e.what();
        throw;
    }
}

bool SqliteUserTodoRepository::update(const dto::UserTodo& todo)
{
    if (!todo.id.has_value())
    {
        LOG_WARN << "update: отсутствует ID задачи";
        return false;
    }

    // Проверяем, что если caption передан, он не пустой
    if (todo.caption.has_value() && todo.caption->empty())
    {
        LOG_WARN << "update: caption не может быть пустой строкой";
        return false;
    }

    try
    {
        auto conn = connection();
        std::vector<std::string> setClauses;
        std::string sql = "UPDATE UserTodo SET ";

        if (todo.userId.has_value())
            setClauses.push_back("userId = :userId");
        if (todo.isDone.has_value())
            setClauses.push_back("isDone = :isDone");
        if (todo.caption.has_value())
        {
            setClauses.push_back("caption = :caption");
            setClauses.push_back("searchCaption = :searchCaption");
        }

        if (setClauses.empty())
        {
            LOG_WARN << "update: нет полей для обновления";
            return false;
        }

        sql += boost::algorithm::join(setClauses, ", ");
        sql += " WHERE id = :id";

        auto stmt = conn->prepareStatement(sql);

        if (todo.userId.has_value())
            stmt->bindInt64("userId", *todo.userId);
        if (todo.isDone.has_value())
            stmt->bindInt64("isDone", *todo.isDone ? 1 : 0);
        if (todo.caption.has_value())
        {
            stmt->bindString("caption", *todo.caption);
            stmt->bindString("searchCaption", common::toLowerCase(*todo.caption));
        }

        stmt->bindInt64("id", *todo.id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка обновления задачи: " << e.what();
        return false;
    }
}

bool SqliteUserTodoRepository::remove(int64_t id)
{
    if (id <= 0)
    {
        LOG_WARN << "remove: неверный идентификатор " << id;
        return false;
    }

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement("DELETE FROM UserTodo WHERE id = :id");
        stmt->bindInt64("id", id);

        int64_t affected = stmt->execute();
        return affected > 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка удаления задачи: " << e.what();
        return false;
    }
}

bool SqliteUserTodoRepository::exists(int64_t id)
{
    if (id <= 0)
        return false;

    try
    {
        auto conn = connection();
        auto stmt = conn->prepareStatement(
            "SELECT 1 FROM UserTodo WHERE id = :id LIMIT 1"
        );

        stmt->bindInt64("id", id);
        auto rs = stmt->executeQuery();
        return rs->next();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка проверки существования задачи: " << e.what();
        return false;
    }
}

} // namespace repositories
} // namespace server
