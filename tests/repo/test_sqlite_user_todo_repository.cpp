#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/user_todo.h"

#include "repo/sqlite/sqlite_user_todo_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserTodoRepositoryFixture
{
    UserTodoRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_user_todo_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем таблицу User
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                firstName TEXT,
                middleName TEXT,
                lastName TEXT,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL,
                needChangePassword INTEGER NOT NULL DEFAULT 1,
                isBlocked INTEGER NOT NULL DEFAULT 0,
                isSuperAdmin INTEGER NOT NULL DEFAULT 0,
                isHidden INTEGER NOT NULL DEFAULT 0
            )
        )");

        // Создаем тестовых пользователей
        conn->execute(R"(
            INSERT INTO User (login, email, passwordHash)
            VALUES ('admin', 'admin@test.local', 'hash')
        )");
        m_adminUserId = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, email, passwordHash)
            VALUES ('user1', 'user1@test.local', 'hash')
        )");
        m_user1Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, email, passwordHash)
            VALUES ('user2', 'user2@test.local', 'hash')
        )");
        m_user2Id = conn->lastInsertId();

        // Создаем таблицу UserTodo
        conn->execute(R"(
            CREATE TABLE UserTodo (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                isDone INTEGER NOT NULL DEFAULT 0,
                caption TEXT NOT NULL,
                searchCaption TEXT,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
            )
        )");

        // Индексы
        conn->execute("CREATE INDEX idx_userTodo_userId ON UserTodo(userId)");
        conn->execute("CREATE INDEX idx_userTodo_isDone ON UserTodo(isDone)");

        m_repository = std::make_unique<repositories::SqliteUserTodoRepository>(m_database);
    }

    dto::UserTodo createTestTodo(
        int64_t userId,
        const std::string& caption = "Test todo",
        bool isDone = false
    )
    {
        dto::UserTodo todo;
        todo.userId = userId;
        todo.caption = caption;
        todo.isDone = isDone;
        return todo;
    }

    ~UserTodoRepositoryFixture()
    {
        m_repository.reset();

        if (m_database)
        {
            m_database->shutdown();
            m_database.reset();
        }

        if (!m_tempDbPath.empty() && std::filesystem::exists(m_tempDbPath))
        {
            std::error_code ec;
            std::filesystem::remove(m_tempDbPath, ec);
        }
    }

    std::filesystem::path m_tempDbPath;
    std::shared_ptr<db::SqliteDatabase> m_database;
    std::unique_ptr<repositories::SqliteUserTodoRepository> m_repository;
    int64_t m_adminUserId = 0;
    int64_t m_user1Id = 0;
    int64_t m_user2Id = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserTodoRepositoryTests, UserTodoRepositoryFixture)

// ============================================================
// Тесты создания задач
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_todo_success)
{
    auto todo = createTestTodo(m_user1Id, "Создать проект", false);
    int64_t todoId = m_repository->create(todo);

    BOOST_CHECK_GT(todoId, 0);
    BOOST_CHECK(m_repository->exists(todoId));

    auto found = m_repository->findById(todoId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->caption, "Создать проект");
    BOOST_CHECK_EQUAL(*found->isDone, false);
}

BOOST_AUTO_TEST_CASE(test_create_done_todo)
{
    auto todo = createTestTodo(m_user1Id, "Завершенная задача", true);
    int64_t todoId = m_repository->create(todo);

    BOOST_CHECK_GT(todoId, 0);

    auto found = m_repository->findById(todoId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Завершенная задача");
    BOOST_CHECK_EQUAL(*found->isDone, true);
}

BOOST_AUTO_TEST_CASE(test_create_todo_missing_required_fields)
{
    dto::UserTodo todo;
    todo.caption = "Todo without userId";

    int64_t todoId = m_repository->create(todo);
    BOOST_CHECK_EQUAL(todoId, 0);

    todo.userId = m_user1Id;
    todo.caption = std::nullopt;

    todoId = m_repository->create(todo);
    BOOST_CHECK_EQUAL(todoId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_todo_empty_caption_fails)
{
    auto todo = createTestTodo(m_user1Id, "");
    int64_t todoId = m_repository->create(todo);
    BOOST_CHECK_EQUAL(todoId, 0);
}

// ============================================================
// Тесты поиска задач
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto todo = createTestTodo(m_user1Id, "Задача для поиска");
    int64_t todoId = m_repository->create(todo);
    BOOST_REQUIRE_GT(todoId, 0);

    auto found = m_repository->findById(todoId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, todoId);
    BOOST_CHECK_EQUAL(*found->caption, "Задача для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    // Задачи для user1
    m_repository->create(createTestTodo(m_user1Id, "Задача 1"));
    m_repository->create(createTestTodo(m_user1Id, "Задача 2"));

    // Задача для user2
    m_repository->create(createTestTodo(m_user2Id, "Задача для user2"));

    auto todos = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(todos.size(), 2);

    for (const auto& todo : todos)
    {
        BOOST_CHECK_EQUAL(*todo.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id_empty)
{
    auto todos = m_repository->findByUserId(99999);
    BOOST_CHECK(todos.empty());
}

// ============================================================
// Тесты findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [todos, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(todos.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 25 задач
    for (int i = 1; i <= 25; ++i)
    {
        m_repository->create(createTestTodo(
            m_user1Id,
            "Задача " + std::to_string(i),
            i % 2 == 0
        ));
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 25);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 10);
    BOOST_CHECK_EQUAL(total2, 25);

    auto [page3, total3] = m_repository->findAll(3, 10);
    BOOST_CHECK_EQUAL(page3.size(), 5);
    BOOST_CHECK_EQUAL(total3, 25);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    m_repository->create(createTestTodo(m_user1Id, "User1 задача 1"));
    m_repository->create(createTestTodo(m_user1Id, "User1 задача 2"));
    m_repository->create(createTestTodo(m_user2Id, "User2 задача 1"));

    auto [todos, total] = m_repository->findAll(1, 20, m_user1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& todo : todos)
    {
        BOOST_CHECK_EQUAL(*todo.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_done_status)
{
    // Создаем задачи с разным статусом
    m_repository->create(createTestTodo(m_user1Id, "Невыполненная задача 1", false));
    m_repository->create(createTestTodo(m_user1Id, "Невыполненная задача 2", false));
    m_repository->create(createTestTodo(m_user1Id, "Выполненная задача 1", true));
    m_repository->create(createTestTodo(m_user1Id, "Выполненная задача 2", true));

    // Фильтр по выполненным
    auto [doneTodos, doneTotal] = m_repository->findAll(1, 20, std::nullopt, true);
    BOOST_CHECK_EQUAL(doneTotal, 2);
    for (const auto& todo : doneTodos)
    {
        BOOST_CHECK_EQUAL(*todo.isDone, true);
    }

    // Фильтр по невыполненным
    auto [notDoneTodos, notDoneTotal] = m_repository->findAll(1, 20, std::nullopt, false);
    BOOST_CHECK_EQUAL(notDoneTotal, 2);
    for (const auto& todo : notDoneTodos)
    {
        BOOST_CHECK_EQUAL(*todo.isDone, false);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user_and_done)
{
    // Задачи user1
    m_repository->create(createTestTodo(m_user1Id, "User1 невыполненная", false));
    m_repository->create(createTestTodo(m_user1Id, "User1 выполненная", true));

    // Задачи user2
    m_repository->create(createTestTodo(m_user2Id, "User2 невыполненная", false));

    auto [todos, total] = m_repository->findAll(1, 20, m_user1Id, true);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*todos[0].caption, "User1 выполненная");
}

// ============================================================
// Тесты обновления задач
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_todo_success)
{
    auto todo = createTestTodo(m_user1Id, "Старая задача", false);
    int64_t todoId = m_repository->create(todo);
    BOOST_REQUIRE_GT(todoId, 0);

    dto::UserTodo updateData;
    updateData.id = todoId;
    updateData.caption = "Обновленная задача";
    updateData.isDone = true;

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(todoId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Обновленная задача");
    BOOST_CHECK_EQUAL(*found->isDone, true);
}

BOOST_AUTO_TEST_CASE(test_update_todo_partial)
{
    auto todo = createTestTodo(m_user1Id, "Оригинальная задача", false);
    int64_t todoId = m_repository->create(todo);

    // Обновляем только статус
    dto::UserTodo updateData;
    updateData.id = todoId;
    updateData.isDone = true;

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(todoId);
    BOOST_CHECK_EQUAL(*found->caption, "Оригинальная задача");
    BOOST_CHECK_EQUAL(*found->isDone, true);

    // Обновляем только название
    updateData.caption = "Новое название";
    updateData.isDone = std::nullopt;

    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(todoId);
    BOOST_CHECK_EQUAL(*found->caption, "Новое название");
    BOOST_CHECK_EQUAL(*found->isDone, true);
}

BOOST_AUTO_TEST_CASE(test_update_todo_nonexistent)
{
    dto::UserTodo updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующая задача";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_todo_empty_caption_fails)
{
    auto todo = createTestTodo(m_user1Id, "Задача с названием");
    int64_t todoId = m_repository->create(todo);

    dto::UserTodo updateData;
    updateData.id = todoId;
    updateData.caption = "";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления задач
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_todo_success)
{
    auto todo = createTestTodo(m_user1Id, "Задача для удаления");
    int64_t todoId = m_repository->create(todo);
    BOOST_REQUIRE_GT(todoId, 0);

    bool result = m_repository->remove(todoId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(todoId));
}

BOOST_AUTO_TEST_CASE(test_remove_todo_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл задачи
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_todo_lifecycle)
{
    // 1. Создание
    auto todo = createTestTodo(m_user1Id, "Жизненный цикл задачи", false);
    int64_t todoId = m_repository->create(todo);
    BOOST_CHECK_GT(todoId, 0);

    // 2. Чтение
    auto found = m_repository->findById(todoId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл задачи");
    BOOST_CHECK_EQUAL(*found->isDone, false);

    // 3. Обновление (отметка о выполнении)
    dto::UserTodo updateData;
    updateData.id = todoId;
    updateData.isDone = true;
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(todoId);
    BOOST_CHECK_EQUAL(*found->isDone, true);

    // 4. Проверка в списке
    auto [todos, total] = m_repository->findAll(1, 20, m_user1Id);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(todos[0].id.value(), todoId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(todoId));
    BOOST_CHECK(!m_repository->exists(todoId));
}

// ============================================================
// Тест поиска по статусу выполнения для пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_user_and_done_status)
{
    // Создаем задачи для user1
    m_repository->create(createTestTodo(m_user1Id, "Невыполненная 1", false));
    m_repository->create(createTestTodo(m_user1Id, "Невыполненная 2", false));
    m_repository->create(createTestTodo(m_user1Id, "Выполненная 1", true));
    m_repository->create(createTestTodo(m_user1Id, "Выполненная 2", true));

    // Задачи для user2
    m_repository->create(createTestTodo(m_user2Id, "User2 невыполненная", false));
    m_repository->create(createTestTodo(m_user2Id, "User2 выполненная", true));

    // Проверка невыполненных задач user1
    auto [notDone, notDoneTotal] = m_repository->findAll(1, 20, m_user1Id, false);
    BOOST_CHECK_EQUAL(notDoneTotal, 2);
    for (const auto& todo : notDone)
    {
        BOOST_CHECK_EQUAL(*todo.isDone, false);
    }

    // Проверка выполненных задач user1
    auto [done, doneTotal] = m_repository->findAll(1, 20, m_user1Id, true);
    BOOST_CHECK_EQUAL(doneTotal, 2);
    for (const auto& todo : done)
    {
        BOOST_CHECK_EQUAL(*todo.isDone, true);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
