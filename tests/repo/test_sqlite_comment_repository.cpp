#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/comment.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_comment_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct CommentRepositoryFixture
{
    CommentRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_comment_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для комментариев
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL,
                needChangePassword INTEGER NOT NULL DEFAULT 1,
                isBlocked INTEGER NOT NULL DEFAULT 0,
                isSuperAdmin INTEGER NOT NULL DEFAULT 0,
                isHidden INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                isDeleted INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Comment (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                itemId INTEGER NOT NULL,
                createdAt INTEGER NOT NULL,
                content TEXT NOT NULL,
                searchContent TEXT,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE
            )
        )");

        // Создаем тестовых пользователей
        for (int i = 1; i <= 3; ++i)
        {
            conn->execute(
                "INSERT INTO User (login, email, passwordHash) "
                "VALUES ('user"
                + std::to_string(i) + "', 'user" + std::to_string(i) + "@mail.local', 'hash')"
            );
        }

        // Создаем тестовые элементы
        for (int i = 1; i <= 3; ++i)
        {
            conn->execute(
                "INSERT INTO Item (caption) VALUES ('Item " + std::to_string(i) + "')"
            );
        }

        m_repository = std::make_unique<repositories::SqliteCommentRepository>(m_database);

        // Сохраняем ID для использования в тестах
        auto stmt = conn->prepareStatement("SELECT id FROM User ORDER BY id");
        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            m_userIds.push_back(rs->valueInt64(0));
        }

        stmt = conn->prepareStatement("SELECT id FROM Item ORDER BY id");
        rs = stmt->executeQuery();
        while (rs->next())
        {
            m_itemIds.push_back(rs->valueInt64(0));
        }
    }

    dto::Comment createTestComment(
        const std::string& content = "Test comment",
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> itemId = std::nullopt
    )
    {
        dto::Comment comment;
        comment.userId = userId.has_value() ? *userId : (m_userIds.empty() ? 1 : m_userIds[0]);
        comment.itemId = itemId.has_value() ? *itemId : (m_itemIds.empty() ? 1 : m_itemIds[0]);
        comment.content = content;
        comment.createdAt = std::chrono::system_clock::now();
        return comment;
    }

    ~CommentRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteCommentRepository> m_repository;
    std::vector<int64_t> m_userIds;
    std::vector<int64_t> m_itemIds;
};

BOOST_FIXTURE_TEST_SUITE(SqliteCommentRepositoryTests, CommentRepositoryFixture)

// ============================================================
// Тесты создания комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_comment_success)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    auto comment = createTestComment("Это тестовый комментарий");
    int64_t commentId = m_repository->create(comment);

    BOOST_CHECK_GT(commentId, 0);

    auto found = m_repository->findById(commentId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "Это тестовый комментарий");
    BOOST_CHECK_EQUAL(*found->userId, *comment.userId);
    BOOST_CHECK_EQUAL(*found->itemId, *comment.itemId);
    BOOST_CHECK(found->createdAt.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_comment_missing_fields)
{
    dto::Comment comment;
    comment.content = "Комментарий без пользователя";

    int64_t commentId = m_repository->create(comment);
    BOOST_CHECK_EQUAL(commentId, 0);

    dto::Comment comment2;
    comment2.userId = 1;
    comment2.content = "Комментарий без элемента";

    int64_t commentId2 = m_repository->create(comment2);
    BOOST_CHECK_EQUAL(commentId2, 0);
}

BOOST_AUTO_TEST_CASE(test_create_comment_empty_content_fails)
{
    auto comment = createTestComment("");
    int64_t commentId = m_repository->create(comment);
    BOOST_CHECK_EQUAL(commentId, 0);
}

// ============================================================
// Тесты поиска комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    auto comment = createTestComment("Комментарий для поиска");
    int64_t commentId = m_repository->create(comment);
    BOOST_REQUIRE_GT(commentId, 0);

    auto found = m_repository->findById(commentId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, commentId);
    BOOST_CHECK_EQUAL(*found->content, "Комментарий для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE_GE(m_itemIds.size(), 2);

    // Добавляем комментарии к первому элементу
    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "Комментарий к элементу 1 #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Добавляем комментарий ко второму элементу
    auto comment = createTestComment(
        "Комментарий к элементу 2",
        m_userIds[0],
        m_itemIds[1]
    );
    m_repository->create(comment);

    auto comments = m_repository->findByItemId(m_itemIds[0]);
    BOOST_CHECK_EQUAL(comments.size(), 3);
    for (const auto& c : comments)
    {
        BOOST_CHECK_EQUAL(*c.itemId, m_itemIds[0]);
    }

    auto comments2 = m_repository->findByItemId(m_itemIds[1]);
    BOOST_CHECK_EQUAL(comments2.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id_sort_order)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    // Добавляем комментарии с небольшими задержками для обеспечения порядка
    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "Комментарий #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // По возрастанию (по умолчанию)
    auto asc = m_repository->findByItemId(m_itemIds[0], true);
    BOOST_CHECK_EQUAL(asc.size(), 3);
    BOOST_CHECK(asc[0].createdAt <= asc[2].createdAt);

    // По убыванию
    auto desc = m_repository->findByItemId(m_itemIds[0], false);
    BOOST_CHECK_EQUAL(desc.size(), 3);
    BOOST_CHECK(desc[0].createdAt >= desc[2].createdAt);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    BOOST_REQUIRE_GE(m_userIds.size(), 2);
    BOOST_REQUIRE(!m_itemIds.empty());

    // Комментарии первого пользователя
    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "Комментарий пользователя 1 #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    // Комментарии второго пользователя
    for (int i = 0; i < 2; ++i)
    {
        auto comment = createTestComment(
            "Комментарий пользователя 2 #" + std::to_string(i),
            m_userIds[1],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    auto comments1 = m_repository->findByUserId(m_userIds[0]);
    BOOST_CHECK_EQUAL(comments1.size(), 3);
    for (const auto& c : comments1)
    {
        BOOST_CHECK_EQUAL(*c.userId, m_userIds[0]);
    }

    auto comments2 = m_repository->findByUserId(m_userIds[1]);
    BOOST_CHECK_EQUAL(comments2.size(), 2);
}

// ============================================================
// Тесты findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [comments, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(comments.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    // Создаем 15 комментариев
    for (int i = 0; i < 15; ++i)
    {
        auto comment = createTestComment(
            "Комментарий #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_item)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE_GE(m_itemIds.size(), 2);

    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "К элементу 1 #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    auto comment2 = createTestComment(
        "К элементу 2",
        m_userIds[0],
        m_itemIds[1]
    );
    m_repository->create(comment2);

    auto [comments, total] = m_repository->findAll(1, 20, m_itemIds[0]);
    BOOST_CHECK_EQUAL(total, 3);
    for (const auto& c : comments)
    {
        BOOST_CHECK_EQUAL(*c.itemId, m_itemIds[0]);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    BOOST_REQUIRE_GE(m_userIds.size(), 2);
    BOOST_REQUIRE(!m_itemIds.empty());

    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "От пользователя 1 #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    for (int i = 0; i < 2; ++i)
    {
        auto comment = createTestComment(
            "От пользователя 2 #" + std::to_string(i),
            m_userIds[1],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    auto [comments, total] = m_repository->findAll(1, 20, std::nullopt, m_userIds[0]);
    BOOST_CHECK_EQUAL(total, 3);
    for (const auto& c : comments)
    {
        BOOST_CHECK_EQUAL(*c.userId, m_userIds[0]);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_date_range)
{
    // Проверяем, что у нас есть пользователь и элемент
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    // Используем репозиторий для создания комментариев
    // Создаем 2 старых комментария с задержкой между ними
    for (int i = 0; i < 2; ++i)
    {
        auto comment = createTestComment(
            "Старый комментарий " + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Ждем 2 секунды, чтобы новые комментарии точно были позже
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Создаем 2 новых комментария
    for (int i = 0; i < 2; ++i)
    {
        auto comment = createTestComment(
            "Новый комментарий " + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Проверяем, что все 4 комментария созданы
    auto [allComments, totalAll] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(totalAll, 4);

    // Проверяем содержимое комментариев
    int oldCount = 0;
    int newCount = 0;
    for (const auto& c : allComments)
    {
        if (c.content->find("Старый") != std::string::npos)
        {
            oldCount++;
        }
        else if (c.content->find("Новый") != std::string::npos)
        {
            newCount++;
        }
    }
    BOOST_CHECK_EQUAL(oldCount, 2);
    BOOST_CHECK_EQUAL(newCount, 2);

    // Получаем все комментарии для элемента через репозиторий
    auto comments = m_repository->findByItemId(m_itemIds[0]);
    BOOST_CHECK_EQUAL(comments.size(), 4);

    // Проверяем, что старые и новые комментарии имеют разные временные метки
    // Находим минимальную и максимальную временные метки
    common::DateTime minTime = *comments[0].createdAt;
    common::DateTime maxTime = *comments[0].createdAt;

    for (const auto& c : comments)
    {
        if (*c.createdAt < minTime)
            minTime = *c.createdAt;
        if (*c.createdAt > maxTime)
            maxTime = *c.createdAt;
    }

    // Разница между самым старым и самым новым комментарием должна быть больше 2 секунд
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(maxTime - minTime);
    BOOST_CHECK_GE(diff.count(), 2);
}

// ============================================================
// Тесты обновления комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_comment_success)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    auto comment = createTestComment("Старый комментарий");
    int64_t commentId = m_repository->create(comment);
    BOOST_REQUIRE_GT(commentId, 0);

    dto::Comment updateData;
    updateData.id = commentId;
    updateData.content = "Обновленный комментарий";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(commentId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "Обновленный комментарий");
}

BOOST_AUTO_TEST_CASE(test_update_comment_empty_content_fails)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    auto comment = createTestComment("Оригинальный комментарий");
    int64_t commentId = m_repository->create(comment);
    BOOST_REQUIRE_GT(commentId, 0);

    dto::Comment updateData;
    updateData.id = commentId;
    updateData.content = "";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_comment_nonexistent)
{
    dto::Comment updateData;
    updateData.id = 99999;
    updateData.content = "Обновление несуществующего комментария";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_comment_success)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    auto comment = createTestComment("Комментарий для удаления");
    int64_t commentId = m_repository->create(comment);
    BOOST_REQUIRE_GT(commentId, 0);

    bool result = m_repository->remove(commentId);
    BOOST_CHECK(result);

    auto found = m_repository->findById(commentId);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_remove_comment_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE_GE(m_itemIds.size(), 2);

    // Комментарии к первому элементу
    for (int i = 0; i < 3; ++i)
    {
        auto comment = createTestComment(
            "К элементу 1 #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    // Комментарий ко второму элементу
    auto comment = createTestComment(
        "К элементу 2",
        m_userIds[0],
        m_itemIds[1]
    );
    m_repository->create(comment);

    int64_t removed = m_repository->removeByItemId(m_itemIds[0]);
    BOOST_CHECK_EQUAL(removed, 3);

    auto comments = m_repository->findByItemId(m_itemIds[0]);
    BOOST_CHECK(comments.empty());

    comments = m_repository->findByItemId(m_itemIds[1]);
    BOOST_CHECK_EQUAL(comments.size(), 1);
}

// ============================================================
// Тесты подсчета комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_count_by_item_id)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE_GE(m_itemIds.size(), 2);

    for (int i = 0; i < 5; ++i)
    {
        auto comment = createTestComment(
            "Комментарий #" + std::to_string(i),
            m_userIds[0],
            m_itemIds[0]
        );
        m_repository->create(comment);
    }

    auto comment2 = createTestComment(
        "Комментарий к другому элементу",
        m_userIds[0],
        m_itemIds[1]
    );
    m_repository->create(comment2);

    int64_t count1 = m_repository->countByItemId(m_itemIds[0]);
    BOOST_CHECK_EQUAL(count1, 5);

    int64_t count2 = m_repository->countByItemId(m_itemIds[1]);
    BOOST_CHECK_EQUAL(count2, 1);

    int64_t count3 = m_repository->countByItemId(99999);
    BOOST_CHECK_EQUAL(count3, 0);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл комментария
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_comment_lifecycle)
{
    BOOST_REQUIRE(!m_userIds.empty());
    BOOST_REQUIRE(!m_itemIds.empty());

    // 1. Создание
    auto comment = createTestComment("Жизненный цикл комментария");
    int64_t commentId = m_repository->create(comment);
    BOOST_CHECK_GT(commentId, 0);

    // 2. Чтение
    auto found = m_repository->findById(commentId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->content, "Жизненный цикл комментария");

    // 3. Обновление
    dto::Comment updateData;
    updateData.id = commentId;
    updateData.content = "Обновленный комментарий";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(commentId);
    BOOST_CHECK_EQUAL(*found->content, "Обновленный комментарий");

    // 4. Проверка в списке
    auto [comments, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(comments[0].id.value(), commentId);

    // 5. Подсчет комментариев
    int64_t count = m_repository->countByItemId(*comment.itemId);
    BOOST_CHECK_EQUAL(count, 1);

    // 6. Удаление
    BOOST_CHECK(m_repository->remove(commentId));
    BOOST_CHECK(!m_repository->exists(commentId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
