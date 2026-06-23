#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/user_notification.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_user_notification_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserNotificationRepositoryFixture
{
    UserNotificationRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_user_notification_repo.db";
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

        // Создаем упрощенную схему для Item (только для тестов)
        conn->execute(R"(
            CREATE TABLE Item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER,
                parentId INTEGER,
                stateId INTEGER,
                phaseId INTEGER,
                caption TEXT NOT NULL,
                content TEXT,
                searchCaption TEXT,
                searchContent TEXT,
                isDeleted INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE UserNotification (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                itemId INTEGER NOT NULL,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
                UNIQUE(userId, itemId)
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

        // Создаем тестовые элементы
        conn->execute(R"(
            INSERT INTO Item (caption)
            VALUES ('Item 1')
        )");
        m_item1Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO Item (caption)
            VALUES ('Item 2')
        )");
        m_item2Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO Item (caption)
            VALUES ('Item 3')
        )");
        m_item3Id = conn->lastInsertId();

        conn->execute(R"(
            INSERT INTO Item (caption)
            VALUES ('Item 4')
        )");
        m_item4Id = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteUserNotificationRepository>(m_database);
    }

    dto::UserNotification createTestNotification(
        int64_t userId,
        int64_t itemId
    )
    {
        dto::UserNotification notification;
        notification.userId = userId;
        notification.itemId = itemId;
        return notification;
    }

    ~UserNotificationRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteUserNotificationRepository> m_repository;
    int64_t m_user1Id = 0;
    int64_t m_user2Id = 0;
    int64_t m_user3Id = 0;
    int64_t m_item1Id = 0;
    int64_t m_item2Id = 0;
    int64_t m_item3Id = 0;
    int64_t m_item4Id = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserNotificationRepositoryTests, UserNotificationRepositoryFixture)

// ============================================================
// Тесты создания подписок
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_notification_success)
{
    auto notification = createTestNotification(m_user1Id, m_item1Id);
    int64_t notifId = m_repository->create(notification);

    BOOST_CHECK_GT(notifId, 0);

    auto found = m_repository->findById(notifId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->itemId, m_item1Id);
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_notification_fails)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));

    auto duplicate = createTestNotification(m_user1Id, m_item1Id);

    // Репозиторий должен бросить исключение при нарушении UNIQUE constraint
    BOOST_CHECK_THROW(
        m_repository->create(duplicate),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_notification_missing_required_fields)
{
    dto::UserNotification notification;
    notification.userId = m_user1Id;
    // itemId отсутствует

    int64_t notifId = m_repository->create(notification);
    BOOST_CHECK_EQUAL(notifId, 0);
}

// ============================================================
// Тесты поиска подписок
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto notification = createTestNotification(m_user1Id, m_item1Id);
    int64_t notifId = m_repository->create(notification);
    BOOST_REQUIRE_GT(notifId, 0);

    auto found = m_repository->findById(notifId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, notifId);
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->itemId, m_item1Id);
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
    auto [notifications, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(notifications.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 25 подписок с уникальными парами (userId, itemId)
    // Используем комбинации: для каждого пользователя разные элементы
    int itemIds[] = { m_item1Id, m_item2Id, m_item3Id, m_item4Id };
    int userIds[] = { m_user1Id, m_user2Id, m_user3Id };

    int count = 0;
    for (int i = 0; i < 3 && count < 25; ++i)
    {
        for (int j = 0; j < 4 && count < 25; ++j)
        {
            m_repository->create(createTestNotification(userIds[i], itemIds[j]));
            count++;
        }
    }

    auto [page1, total1] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total1, 12); // 3 * 4 = 12
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 2);
    BOOST_CHECK_EQUAL(total2, 12);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));

    auto [notifications, total] = m_repository->findAll(1, 20, m_user1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& n : notifications)
    {
        BOOST_CHECK_EQUAL(*n.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_item)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));

    auto [notifications, total] = m_repository->findAll(1, 20, std::nullopt, m_item1Id);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& n : notifications)
    {
        BOOST_CHECK_EQUAL(*n.itemId, m_item1Id);
    }
}

// ============================================================
// Тесты поиска подписок пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_user)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));
    m_repository->create(createTestNotification(m_user1Id, m_item3Id));

    auto notifications = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(notifications.size(), 3);

    for (const auto& n : notifications)
    {
        BOOST_CHECK_EQUAL(*n.userId, m_user1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_user_empty)
{
    auto notifications = m_repository->findByUserId(99999);
    BOOST_CHECK(notifications.empty());
}

// ============================================================
// Тесты поиска подписчиков элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_item)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));
    m_repository->create(createTestNotification(m_user3Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));

    auto notifications = m_repository->findByItemId(m_item1Id);
    BOOST_CHECK_EQUAL(notifications.size(), 3);

    for (const auto& n : notifications)
    {
        BOOST_CHECK_EQUAL(*n.itemId, m_item1Id);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_item_empty)
{
    auto notifications = m_repository->findByItemId(99999);
    BOOST_CHECK(notifications.empty());
}

// ============================================================
// Тесты поиска подписки по пользователю и элементу
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_user_and_item_success)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));

    auto found = m_repository->findByUserAndItem(m_user1Id, m_item1Id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->itemId, m_item1Id);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_and_item_not_found)
{
    auto found = m_repository->findByUserAndItem(m_user1Id, m_item1Id);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты проверки существования подписки
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));

    bool exists = m_repository->exists(m_user1Id, m_item1Id);
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    bool exists = m_repository->exists(m_user1Id, m_item1Id);
    BOOST_CHECK(!exists);
}

// ============================================================
// Тесты удаления подписок
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_by_id_success)
{
    auto notification = createTestNotification(m_user1Id, m_item1Id);
    int64_t notifId = m_repository->create(notification);
    BOOST_REQUIRE_GT(notifId, 0);

    bool result = m_repository->remove(notifId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(m_user1Id, m_item1Id));
}

BOOST_AUTO_TEST_CASE(test_remove_by_id_not_found)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_user_and_item_success)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));

    bool result = m_repository->removeByUserAndItem(m_user1Id, m_item1Id);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(m_user1Id, m_item1Id));
}

BOOST_AUTO_TEST_CASE(test_remove_by_user_and_item_not_found)
{
    bool result = m_repository->removeByUserAndItem(m_user1Id, m_item1Id);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты массового удаления
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_by_user)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));
    m_repository->create(createTestNotification(m_user1Id, m_item3Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));

    int64_t deleted = m_repository->removeByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(deleted, 3);

    auto notifications = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK(notifications.empty());

    auto user2Notifications = m_repository->findByUserId(m_user2Id);
    BOOST_CHECK_EQUAL(user2Notifications.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_remove_by_item)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));
    m_repository->create(createTestNotification(m_user3Id, m_item1Id));
    m_repository->create(createTestNotification(m_user1Id, m_item2Id));

    int64_t deleted = m_repository->removeByItemId(m_item1Id);
    BOOST_CHECK_EQUAL(deleted, 3);

    auto notifications = m_repository->findByItemId(m_item1Id);
    BOOST_CHECK(notifications.empty());

    auto item2Notifications = m_repository->findByItemId(m_item2Id);
    BOOST_CHECK_EQUAL(item2Notifications.size(), 1);
}

// ============================================================
// Тесты получения ID подписчиков
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_subscriber_ids)
{
    m_repository->create(createTestNotification(m_user1Id, m_item1Id));
    m_repository->create(createTestNotification(m_user2Id, m_item1Id));
    m_repository->create(createTestNotification(m_user3Id, m_item1Id));

    auto subscriberIds = m_repository->getSubscriberIds(m_item1Id);
    BOOST_CHECK_EQUAL(subscriberIds.size(), 3);

    std::sort(subscriberIds.begin(), subscriberIds.end());
    BOOST_CHECK_EQUAL(subscriberIds[0], m_user1Id);
    BOOST_CHECK_EQUAL(subscriberIds[1], m_user2Id);
    BOOST_CHECK_EQUAL(subscriberIds[2], m_user3Id);
}

BOOST_AUTO_TEST_CASE(test_get_subscriber_ids_empty)
{
    auto subscriberIds = m_repository->getSubscriberIds(99999);
    BOOST_CHECK(subscriberIds.empty());
}

// ============================================================
// Интеграционный тест: полный жизненный цикл подписки
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_notification_lifecycle)
{
    // 1. Создание
    auto notification = createTestNotification(m_user1Id, m_item1Id);
    int64_t notifId = m_repository->create(notification);
    BOOST_CHECK_GT(notifId, 0);

    // 2. Чтение
    auto found = m_repository->findById(notifId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_user1Id);
    BOOST_CHECK_EQUAL(*found->itemId, m_item1Id);

    // 3. Проверка существования
    BOOST_CHECK(m_repository->exists(m_user1Id, m_item1Id));

    // 4. Проверка в списке пользователя
    auto userNotifications = m_repository->findByUserId(m_user1Id);
    BOOST_CHECK_EQUAL(userNotifications.size(), 1);
    BOOST_CHECK_EQUAL(*userNotifications[0].id, notifId);

    // 5. Проверка в списке подписчиков элемента
    auto subscriberIds = m_repository->getSubscriberIds(m_item1Id);
    BOOST_CHECK_EQUAL(subscriberIds.size(), 1);
    BOOST_CHECK_EQUAL(subscriberIds[0], m_user1Id);

    // 6. Удаление
    BOOST_CHECK(m_repository->remove(notifId));
    BOOST_CHECK(!m_repository->exists(m_user1Id, m_item1Id));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
