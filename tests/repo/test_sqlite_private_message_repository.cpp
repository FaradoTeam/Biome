#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/private_message.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_private_message_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct PrivateMessageRepositoryFixture
{
    PrivateMessageRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_private_message_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для PrivateMessage
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
            CREATE TABLE PrivateMessage (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                senderUserId INTEGER NOT NULL,
                receiverUserId INTEGER NOT NULL,
                creationTimestamp INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
                content TEXT NOT NULL,
                isViewed INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (senderUserId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (receiverUserId) REFERENCES User(id) ON DELETE CASCADE
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

        m_repository = std::make_unique<repositories::SqlitePrivateMessageRepository>(m_database);
    }

    dto::PrivateMessage createTestMessage(
        int64_t senderUserId,
        int64_t receiverUserId,
        const std::string& content = "Test message content",
        bool isViewed = false
    )
    {
        dto::PrivateMessage msg;
        msg.senderUserId = senderUserId;
        msg.receiverUserId = receiverUserId;
        msg.content = content;
        msg.isViewed = isViewed;
        msg.creationTimestamp = std::chrono::system_clock::now();
        return msg;
    }

    ~PrivateMessageRepositoryFixture()
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
    std::unique_ptr<repositories::SqlitePrivateMessageRepository> m_repository;
    int64_t m_user1Id = 0;
    int64_t m_user2Id = 0;
    int64_t m_user3Id = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqlitePrivateMessageRepositoryTests, PrivateMessageRepositoryFixture)

// ============================================================
// Тесты создания сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_message_success)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Hello, User2!");
    int64_t msgId = m_repository->create(msg);

    BOOST_CHECK_GT(msgId, 0);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->senderUserId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->receiverUserId, m_user2Id);
    BOOST_CHECK_EQUAL(*found->content, "Hello, User2!");
    BOOST_CHECK_EQUAL(*found->isViewed, false);
    BOOST_CHECK(found->creationTimestamp.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_message_missing_required_fields)
{
    dto::PrivateMessage msg;
    msg.senderUserId = m_user1Id;
    msg.receiverUserId = m_user2Id;
    // content отсутствует

    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_EQUAL(msgId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_message_with_empty_content)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "");
    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_EQUAL(msgId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_message_with_viewed_flag)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Viewed message", true);
    int64_t msgId = m_repository->create(msg);

    BOOST_CHECK_GT(msgId, 0);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isViewed, true);
}

// ============================================================
// Тесты поиска сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Message for search");
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
    // Создаем 25 сообщений между user1 и user2
    for (int i = 1; i <= 25; ++i)
    {
        m_repository->create(createTestMessage(
            m_user1Id,
            m_user2Id,
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

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    // Сообщения между user1 и user2
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "User1 -> User2"));
    m_repository->create(createTestMessage(m_user2Id, m_user1Id, "User2 -> User1"));

    // Сообщения между user1 и user3
    m_repository->create(createTestMessage(m_user1Id, m_user3Id, "User1 -> User3"));

    auto [messages, total] = m_repository->findAll(1, 20, m_user1Id);
    BOOST_CHECK_EQUAL(total, 3);

    for (const auto& msg : messages)
    {
        BOOST_CHECK(*msg.senderUserId == m_user1Id || *msg.receiverUserId == m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_viewed)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Viewed 1", true));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Viewed 2", true));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Unviewed 1", false));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Unviewed 2", false));

    auto [viewed, totalViewed] = m_repository->findAll(1, 20, std::nullopt, true);
    BOOST_CHECK_EQUAL(totalViewed, 2);
    for (const auto& msg : viewed)
    {
        BOOST_CHECK_EQUAL(*msg.isViewed, true);
    }

    auto [unviewed, totalUnviewed] = m_repository->findAll(1, 20, std::nullopt, false);
    BOOST_CHECK_EQUAL(totalUnviewed, 2);
    for (const auto& msg : unviewed)
    {
        BOOST_CHECK_EQUAL(*msg.isViewed, false);
    }
}

// ============================================================
// Тесты переписки (conversation)
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_conversation)
{
    // Сообщения между user1 и user2
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "User1: Hello"));
    m_repository->create(createTestMessage(m_user2Id, m_user1Id, "User2: Hi!"));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "User1: How are you?"));

    // Сообщение с другим пользователем (не должно попасть в переписку)
    m_repository->create(createTestMessage(m_user1Id, m_user3Id, "User1: Hello User3"));

    auto conversation = m_repository->findConversation(m_user1Id, m_user2Id);
    BOOST_CHECK_EQUAL(conversation.size(), 3);

    // Проверяем порядок (по возрастанию времени)
    for (size_t i = 1; i < conversation.size(); ++i)
    {
        BOOST_CHECK(conversation[i - 1].creationTimestamp <= conversation[i].creationTimestamp);
    }
}

// ============================================================
// Тесты поиска по отправителю и получателю
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_sender)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "From User1"));
    m_repository->create(createTestMessage(m_user1Id, m_user3Id, "From User1 to User3"));
    m_repository->create(createTestMessage(m_user2Id, m_user1Id, "From User2"));

    auto sent = m_repository->findBySender(m_user1Id);
    BOOST_CHECK_EQUAL(sent.size(), 2);

    for (const auto& msg : sent)
    {
        BOOST_CHECK_EQUAL(*msg.senderUserId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_receiver)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "To User2 1"));
    m_repository->create(createTestMessage(m_user3Id, m_user2Id, "To User2 2"));
    m_repository->create(createTestMessage(m_user1Id, m_user3Id, "To User3"));

    auto received = m_repository->findByReceiver(m_user2Id);
    BOOST_CHECK_EQUAL(received.size(), 2);

    for (const auto& msg : received)
    {
        BOOST_CHECK_EQUAL(*msg.receiverUserId, m_user2Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_receiver_only_unviewed)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Viewed", true));
    m_repository->create(createTestMessage(m_user3Id, m_user2Id, "Unviewed 1", false));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Unviewed 2", false));

    auto unviewed = m_repository->findByReceiver(m_user2Id, true);
    BOOST_CHECK_EQUAL(unviewed.size(), 2);

    for (const auto& msg : unviewed)
    {
        BOOST_CHECK_EQUAL(*msg.isViewed, false);
        BOOST_CHECK_EQUAL(*msg.receiverUserId, m_user2Id);
    }
}

// ============================================================
// Тесты обновления сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_message_mark_as_viewed)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Unviewed message", false);
    int64_t msgId = m_repository->create(msg);
    BOOST_REQUIRE_GT(msgId, 0);

    dto::PrivateMessage updateData;
    updateData.id = msgId;
    updateData.isViewed = true;

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isViewed, true);
    BOOST_CHECK_EQUAL(*found->content, "Unviewed message");
}

BOOST_AUTO_TEST_CASE(test_update_message_content)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Old content", false);
    int64_t msgId = m_repository->create(msg);
    BOOST_REQUIRE_GT(msgId, 0);

    dto::PrivateMessage updateData;
    updateData.id = msgId;
    updateData.content = "New content";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "New content");
    BOOST_CHECK_EQUAL(*found->isViewed, false);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_message)
{
    dto::PrivateMessage updateData;
    updateData.id = 99999;
    updateData.isViewed = true;

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_message_success)
{
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Message to delete");
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
// Тесты отметки всех сообщений как прочитанных
// ============================================================

BOOST_AUTO_TEST_CASE(test_mark_all_as_viewed)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 1", false));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 2", false));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 3", false));

    int64_t updated = m_repository->markAllAsViewed(m_user1Id, m_user2Id);
    BOOST_CHECK_EQUAL(updated, 3);

    auto messages = m_repository->findByReceiver(m_user2Id);
    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.isViewed, true);
    }
}

BOOST_AUTO_TEST_CASE(test_mark_all_as_viewed_only_unviewed)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Already viewed", true));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Unviewed 1", false));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Unviewed 2", false));

    int64_t updated = m_repository->markAllAsViewed(m_user1Id, m_user2Id);
    BOOST_CHECK_EQUAL(updated, 2);

    auto messages = m_repository->findByReceiver(m_user2Id);
    for (const auto& msg : messages)
    {
        BOOST_CHECK_EQUAL(*msg.isViewed, true);
    }
}

BOOST_AUTO_TEST_CASE(test_mark_all_as_viewed_empty)
{
    // Сообщения между другими пользователями
    m_repository->create(createTestMessage(m_user1Id, m_user3Id, "Different"));

    int64_t updated = m_repository->markAllAsViewed(m_user1Id, m_user2Id);
    BOOST_CHECK_EQUAL(updated, 0);
}

// ============================================================
// Тесты подсчёта непрочитанных сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_count_unviewed)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 1", true));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 2", false));
    m_repository->create(createTestMessage(m_user3Id, m_user2Id, "Msg 3", false));

    int64_t count = m_repository->countUnviewed(m_user2Id);
    BOOST_CHECK_EQUAL(count, 2);
}

BOOST_AUTO_TEST_CASE(test_count_unviewed_zero)
{
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 1", true));
    m_repository->create(createTestMessage(m_user1Id, m_user2Id, "Msg 2", true));

    int64_t count = m_repository->countUnviewed(m_user2Id);
    BOOST_CHECK_EQUAL(count, 0);
}

BOOST_AUTO_TEST_CASE(test_count_unviewed_no_messages)
{
    int64_t count = m_repository->countUnviewed(m_user2Id);
    BOOST_CHECK_EQUAL(count, 0);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл сообщения
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_message_lifecycle)
{
    // 1. Создание
    auto msg = createTestMessage(m_user1Id, m_user2Id, "Hello world!");
    int64_t msgId = m_repository->create(msg);
    BOOST_CHECK_GT(msgId, 0);

    // 2. Чтение
    auto found = m_repository->findById(msgId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "Hello world!");
    BOOST_CHECK_EQUAL(*found->isViewed, false);

    // 3. Отметка о прочтении
    dto::PrivateMessage updateData;
    updateData.id = msgId;
    updateData.isViewed = true;
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(msgId);
    BOOST_CHECK_EQUAL(*found->isViewed, true);

    // 4. Проверка в списке
    auto [messages, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*messages[0].id, msgId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(msgId));
    BOOST_CHECK(!m_repository->exists(msgId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
