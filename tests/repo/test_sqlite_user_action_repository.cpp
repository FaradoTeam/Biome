#include <chrono>
#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/user_action.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_user_action_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserActionRepositoryFixture
{
    UserActionRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_user_action_repo.db";
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
            INSERT INTO User (login, email, passwordHash, isSuperAdmin)
            VALUES ('admin', 'admin@test.local', 'hash', 1)
        )");
        m_adminUserId = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, email, passwordHash, isSuperAdmin)
            VALUES ('user1', 'user1@test.local', 'hash', 0)
        )");
        m_user1Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, email, passwordHash, isSuperAdmin)
            VALUES ('user2', 'user2@test.local', 'hash', 0)
        )");
        m_user2Id = conn->lastInsertId();

        // Создаем таблицу UserAction
        conn->execute(R"(
            CREATE TABLE UserAction (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                timestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
                caption TEXT NOT NULL,
                description TEXT,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
            )
        )");

        // Индексы
        conn->execute("CREATE INDEX idx_userAction_userId ON UserAction(userId)");
        conn->execute("CREATE INDEX idx_userAction_timestamp ON UserAction(timestamp)");

        m_repository = std::make_unique<repositories::SqliteUserActionRepository>(m_database);
    }

    dto::UserAction createTestAction(
        int64_t userId,
        const std::string& caption = "Test action",
        const std::string& description = "Test description"
    )
    {
        dto::UserAction action;
        action.userId = userId;
        action.caption = caption;
        action.description = description;
        action.timestamp = std::chrono::system_clock::now();
        return action;
    }

    ~UserActionRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteUserActionRepository> m_repository;
    int64_t m_adminUserId = 0;
    int64_t m_user1Id = 0;
    int64_t m_user2Id = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserActionRepositoryTests, UserActionRepositoryFixture)

// ============================================================
// Тесты создания действий
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_action_success)
{
    auto action = createTestAction(m_user1Id, "Создание проекта", "Пользователь создал новый проект");
    int64_t actionId = m_repository->create(action);

    BOOST_CHECK_GT(actionId, 0);
    BOOST_CHECK(m_repository->exists(actionId));

    auto found = m_repository->findById(actionId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->caption, "Создание проекта");
    BOOST_CHECK_EQUAL(*found->description, "Пользователь создал новый проект");
    BOOST_CHECK(found->timestamp.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_action_without_description)
{
    auto action = createTestAction(m_user1Id, "Вход в систему");
    action.description = std::nullopt;

    int64_t actionId = m_repository->create(action);
    BOOST_CHECK_GT(actionId, 0);

    auto found = m_repository->findById(actionId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Вход в систему");
    BOOST_CHECK(!found->description.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_action_missing_required_fields)
{
    dto::UserAction action;
    action.caption = "Action without userId";

    int64_t actionId = m_repository->create(action);
    BOOST_CHECK_EQUAL(actionId, 0);

    action.userId = m_user1Id;
    action.caption = std::nullopt;

    actionId = m_repository->create(action);
    BOOST_CHECK_EQUAL(actionId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_action_empty_caption_fails)
{
    auto action = createTestAction(m_user1Id, "");
    int64_t actionId = m_repository->create(action);
    BOOST_CHECK_EQUAL(actionId, 0);
}

// ============================================================
// Тесты поиска действий
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto action = createTestAction(m_user1Id, "Действие для поиска");
    int64_t actionId = m_repository->create(action);
    BOOST_REQUIRE_GT(actionId, 0);

    auto found = m_repository->findById(actionId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, actionId);
    BOOST_CHECK_EQUAL(*found->caption, "Действие для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    // Действия для user1
    m_repository->create(createTestAction(m_user1Id, "Действие 1"));
    m_repository->create(createTestAction(m_user1Id, "Действие 2"));

    // Действие для user2
    m_repository->create(createTestAction(m_user2Id, "Действие для user2"));

    auto actions = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(actions.size(), 2);

    for (const auto& action : actions)
    {
        BOOST_CHECK_EQUAL(*action.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id_empty)
{
    auto actions = m_repository->findByUserId(99999);
    BOOST_CHECK(actions.empty());
}

// ============================================================
// Тесты findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [actions, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(actions.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 25 действий
    for (int i = 1; i <= 25; ++i)
    {
        m_repository->create(createTestAction(
            m_user1Id,
            "Действие " + std::to_string(i),
            "Описание действия " + std::to_string(i)
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
    m_repository->create(createTestAction(m_user1Id, "User1 действие 1"));
    m_repository->create(createTestAction(m_user1Id, "User1 действие 2"));
    m_repository->create(createTestAction(m_user2Id, "User2 действие 1"));

    auto [actions, total] = m_repository->findAll(1, 20, m_user1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& action : actions)
    {
        BOOST_CHECK_EQUAL(*action.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_date_range)
{
    using namespace std::chrono;

    // Создаем действия с разными датами
    auto now = system_clock::now();

    auto action1 = createTestAction(m_user1Id, "Действие 1");
    action1.timestamp = now - hours(5);
    m_repository->create(action1);

    auto action2 = createTestAction(m_user1Id, "Действие 2");
    action2.timestamp = now - hours(2);
    m_repository->create(action2);

    auto action3 = createTestAction(m_user1Id, "Действие 3");
    action3.timestamp = now + hours(1);
    m_repository->create(action3);

    // Ищем действия за последние 3 часа
    auto dateFrom = now - hours(3);
    auto dateTo = now + hours(2);

    auto [actions, total] = m_repository->findAll(1, 20, std::nullopt, dateFrom, dateTo);
    BOOST_CHECK_EQUAL(total, 2);

    for (const auto& action : actions)
    {
        BOOST_CHECK(*action.timestamp >= dateFrom);
        BOOST_CHECK(*action.timestamp <= dateTo);
    }
}

// ============================================================
// Тесты удаления действий
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_action_success)
{
    auto action = createTestAction(m_user1Id, "Действие для удаления");
    int64_t actionId = m_repository->create(action);
    BOOST_REQUIRE_GT(actionId, 0);

    bool result = m_repository->remove(actionId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(actionId));
}

BOOST_AUTO_TEST_CASE(test_remove_action_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл действия
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_action_lifecycle)
{
    // 1. Создание
    auto action = createTestAction(m_user1Id, "Жизненный цикл действия", "Полный цикл тестирования");
    int64_t actionId = m_repository->create(action);
    BOOST_CHECK_GT(actionId, 0);

    // 2. Чтение
    auto found = m_repository->findById(actionId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл действия");
    BOOST_CHECK_EQUAL(*found->description, "Полный цикл тестирования");

    // 3. Проверка в списке пользователя
    auto userActions = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(userActions.size(), 1);
    BOOST_CHECK_EQUAL(userActions[0].id.value(), actionId);

    // 4. Удаление
    BOOST_CHECK(m_repository->remove(actionId));
    BOOST_CHECK(!m_repository->exists(actionId));
}

// ============================================================
// Тест сортировки по времени
// ============================================================

BOOST_AUTO_TEST_CASE(test_actions_ordered_by_timestamp_desc)
{
    using namespace std::chrono;

    auto now = system_clock::now();

    // Создаем действия с разными временными метками
    auto action1 = createTestAction(m_user1Id, "Самое старое");
    action1.timestamp = now - hours(10);
    m_repository->create(action1);

    auto action2 = createTestAction(m_user1Id, "Среднее");
    action2.timestamp = now - hours(5);
    m_repository->create(action2);

    auto action3 = createTestAction(m_user1Id, "Самое новое");
    action3.timestamp = now - hours(1);
    m_repository->create(action3);

    auto [actions, total] = m_repository->findAll(1, 20, m_user1Id);

    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(*actions[0].caption, "Самое новое");
    BOOST_CHECK_EQUAL(*actions[1].caption, "Среднее");
    BOOST_CHECK_EQUAL(*actions[2].caption, "Самое старое");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
