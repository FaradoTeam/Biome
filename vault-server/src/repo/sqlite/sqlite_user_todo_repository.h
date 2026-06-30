#pragma once

#include <memory>

#include "../user_todo_repository.h"

namespace db
{
class IDatabase;
class IConnection;
class IResultSet;
}

namespace server
{
namespace repositories
{

/**
 * @brief SQLite-реализация репозитория для работы с задачами пользователя.
 */
class SqliteUserTodoRepository final : public IUserTodoRepository
{
public:
    explicit SqliteUserTodoRepository(std::shared_ptr<db::IDatabase> database);
    ~SqliteUserTodoRepository() override = default;

    SqliteUserTodoRepository(const SqliteUserTodoRepository&) = delete;
    SqliteUserTodoRepository& operator=(const SqliteUserTodoRepository&) = delete;

    // IUserTodoRepository
    UserTodosPage findAll(
        int page,
        int pageSize,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<bool> isDone = std::nullopt
    ) override;

    std::optional<dto::UserTodo> findById(int64_t id) override;
    std::vector<dto::UserTodo> findByUserId(int64_t userId) override;
    int64_t create(const dto::UserTodo& todo) override;
    bool update(const dto::UserTodo& todo) override;
    bool remove(int64_t id) override;
    bool exists(int64_t id) override;

    std::shared_ptr<db::IConnection> connection() const;

private:
    dto::UserTodo mapRowToUserTodo(db::IResultSet& rs) const;

private:
    std::shared_ptr<db::IDatabase> m_database;
};

} // namespace repositories
} // namespace server
