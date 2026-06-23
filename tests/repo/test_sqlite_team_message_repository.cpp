#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/team_message.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_team_message_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct TeamMessageRepositoryFixture
{
    TeamMessageRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_team_message_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему
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

        conn->execute(R"(
            CREATE TABLE Team (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT
            )
        )");

        conn->execute(R"(
            CREATE TABLE TeamMessage (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                senderUserId INTEGER NOT NULL,
                teamId INTEGER NOT NULL,
                creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
                content TEXT NOT NULL,
                FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE
            )
        )");

        // Создаем тестовых пользователей
        conn->execute(R"(
            INSERT INTO User (login, firstName, email, passwordHash)
            VALUES ('user1', 'User One', 'user1@test.local', 'hash1')
        )");
        m_user1Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, firstName, email, passwordHash)
            VALUES ('user2', 'User Two', 'user2@test.local', 'hash2')
        )");
        m_user2Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO User (login, firstName, email, passwordHash)
            VALUES ('user3', 'User Three', 'user3@test.local', 'hash3')
        )");
        m_user3Id = conn->lastInsertId();

        // Создаем тестовые команды
        conn->execute(R"(
            INSERT INTO Team (caption, description)
            VALUES ('Team Alpha', 'First test team')
        )");
        m_team1Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO Team (caption, description)
            VALUES ('Team Beta', 'Second test team')
        )");
        m_team2Id = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteTeamMessageRepository>(m_database);
    }

    dto::TeamMessage createTestMessage(
        int64_t senderUserId,
        int64_t teamId,
        const std::string& content = "Test team message"
    )
    {
        dto::TeamMessage msg;
        msg.senderUserId = senderUserId;
        msg.teamId = teamId;
        msg.content = content;
        msg.creationTimestamp = std::chrono::system_clock::now();
        return msg;
    }

    ~TeamMessageRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteTeamMessageRepository> m_repository;
    int64_t m_user1Id = 0;
    int64_t m_user2Id = 0;
    int64_t m_user3Id = 0;
    int64_t m_team1Id = 0;
    int64_t m_team2Id = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteTeamMessageRepositoryTests, TeamMessageRepositoryFixture)

// ============================================================
// Тесты создания сообщений в команде
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_message_success)
{
    auto msg = createTestMessage(m_user1Id, m_team1Id, "Hello Team Alpha!");
    int64_t msgId = m_repository->create(msg);

    BOOST_CHECK_GT(msgId, 0);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->senderUserId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->teamId, m_team1Id);
    BOOST_CHECK_EQUAL(*found->content, "Hello Team Alpha!");
    BOOST_CHECK(found->creationTimestamp.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_message_missing_required_fields)
{
    dto::TeamMessage msg;
    msg.senderUserId = m_user1Id;
    msg.teamId = m_team1Id;
    // content отсутствует

    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_EQUAL(msgId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_message_with_empty_content)
{
    auto msg = createTestMessage(m_user1Id, m_team1Id, "");
    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_EQUAL(msgId, 0);
}

// ============================================================
// Тесты поиска сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto msg = createTestMessage(m_user1Id, m_team1Id, "Message for search");
    int64_t msgId = m_repository->create(msg);
    BOOST_REQUIRE_GT(msgId, 0);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, msgId);
    BOOST_CHECK_EQUAL(*found->content, "Message for search");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [messages, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(messages.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 25 сообщений в команде
    for (int i = 1; i <= 25; ++i)
    {
        m_repository->create(createTestMessage(
            m_user1Id,
            m_team1Id,
            "Message " + std::to_string(i)
        ));
    }

    auto [page1, total1] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total1, 25);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 10);

    auto [page3, total3] = m_repository->findAll(3, 10);
    BOOST_CHECK_EQUAL(page3.size(), 5);
    BOOST_CHECK_EQUAL(total3, 25);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_team)
{
    // Сообщения в Team Alpha
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "Alpha 1"));
    m_repository->create(createTestMessage(m_user2Id, m_team1Id, "Alpha 2"));

    // Сообщения в Team Beta
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "Beta 1"));

    auto [messages, total] = m_repository->findAll(1, 20, m_team1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.teamId, m_team1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_sender)
{
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "From User1 1"));
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "From User1 2"));
    m_repository->create(createTestMessage(m_user2Id, m_team1Id, "From User2"));

    auto [messages, total] = m_repository->findAll(1, 20, std::nullopt, m_user1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.senderUserId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_team_and_sender)
{
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "User1 in Team1"));
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "User1 in Team2"));
    m_repository->create(createTestMessage(m_user2Id, m_team1Id, "User2 in Team1"));

    auto [messages, total] = m_repository->findAll(1, 20, m_team1Id, m_user1Id);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*messages[0].content, "User1 in Team1");
}

// ============================================================
// Тесты поиска по команде
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_team)
{
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "Team1 msg 1"));
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "Team1 msg 2"));
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "Team2 msg"));

    auto messages = m_repository->findByTeamId(m_team1Id);
    BOOST_CHECK_EQUAL(messages.size(), 2);

    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.teamId, m_team1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_team_empty)
{
    auto messages = m_repository->findByTeamId(99999);
    BOOST_CHECK(messages.empty());
}

// ============================================================
// Тесты поиска по отправителю и команде
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_sender_and_team)
{
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "User1 Team1 1"));
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "User1 Team1 2"));
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "User1 Team2"));
    m_repository->create(createTestMessage(m_user2Id, m_team1Id, "User2 Team1"));

    auto messages = m_repository->findBySenderAndTeam(m_user1Id, m_team1Id);
    BOOST_CHECK_EQUAL(messages.size(), 2);

    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.senderUserId, m_user1Id);
        BOOST_CHECK_EQUAL(*msg.teamId, m_team1Id);
    }
}

// ============================================================
// Тесты обновления сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_message_content)
{
    auto msg = createTestMessage(m_user1Id, m_team1Id, "Old content");
    int64_t msgId = m_repository->create(msg);
    BOOST_REQUIRE_GT(msgId, 0);

    dto::TeamMessage updateData;
    updateData.id = msgId;
    updateData.content = "New content";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "New content");
    BOOST_CHECK_EQUAL(*found->senderUserId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->teamId, m_team1Id);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_message)
{
    dto::TeamMessage updateData;
    updateData.id = 99999;
    updateData.content = "New content";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_message_success)
{
    auto msg = createTestMessage(m_user1Id, m_team1Id, "Message to delete");
    int64_t msgId = m_repository->create(msg);
    BOOST_REQUIRE_GT(msgId, 0);

    bool result = m_repository->remove(msgId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(msgId));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent_message)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты массового удаления сообщений команды
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_by_team)
{
    m_repository->create(createTestMessage(m_user1Id, m_team1Id, "Team1 msg 1"));
    m_repository->create(createTestMessage(m_user2Id, m_team1Id, "Team1 msg 2"));
    m_repository->create(createTestMessage(m_user1Id, m_team2Id, "Team2 msg"));

    int64_t deleted = m_repository->removeByTeamId(m_team1Id);
    BOOST_CHECK_EQUAL(deleted, 2);

    auto messages = m_repository->findByTeamId(m_team1Id);
    BOOST_CHECK(messages.empty());

    auto team2Messages = m_repository->findByTeamId(m_team2Id);
    BOOST_CHECK_EQUAL(team2Messages.size(), 1);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл сообщения в команде
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_message_lifecycle)
{
    // 1. Создание
    auto msg = createTestMessage(m_user1Id, m_team1Id, "Hello team!");
    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_GT(msgId, 0);

    // 2. Чтение
    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "Hello team!");

    // 3. Обновление
    dto::TeamMessage updateData;
    updateData.id = msgId;
    updateData.content = "Hello team! Updated!";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(msgId);
    BOOST_CHECK_EQUAL(*found->content, "Hello team! Updated!");

    // 4. Проверка в списке команды
    auto messages = m_repository->findByTeamId(m_team1Id);
    BOOST_CHECK_EQUAL(messages.size(), 1);
    BOOST_CHECK_EQUAL(*messages[0].id, msgId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(msgId));
    BOOST_CHECK(!m_repository->exists(msgId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
