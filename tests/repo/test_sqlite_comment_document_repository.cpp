#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/comment_document.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_comment_document_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct CommentDocumentRepositoryFixture
{
    CommentDocumentRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_comment_document_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем минимальную схему
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL
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
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE Document (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                path TEXT NOT NULL UNIQUE,
                filename TEXT NOT NULL,
                size INTEGER NOT NULL,
                uploadedAt INTEGER NOT NULL,
                uploadedByUserId INTEGER NOT NULL,
                FOREIGN KEY (uploadedByUserId) REFERENCES User(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE CommentDocument (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                commentId INTEGER NOT NULL,
                documentId INTEGER NOT NULL,
                FOREIGN KEY (commentId) REFERENCES Comment(id) ON DELETE CASCADE,
                FOREIGN KEY (documentId) REFERENCES Document(id) ON DELETE CASCADE,
                UNIQUE(commentId, documentId)
            )
        )");

        // Создаем тестового пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('test_user', 'test@mail.local', 'hash')"
        );
        m_testUserId = conn->lastInsertId();

        // Создаем тестовый элемент
        conn->execute(
            "INSERT INTO Item (caption) VALUES ('Test Item')"
        );
        m_testItemId = conn->lastInsertId();

        // Создаем тестовые комментарии (15 штук для тестов пагинации)
        for (int i = 1; i <= 15; ++i)
        {
            conn->execute(
                "INSERT INTO Comment (userId, itemId, createdAt, content) "
                "VALUES ("
                + std::to_string(m_testUserId) + ", " + std::to_string(m_testItemId) + ", strftime('%s', 'now'), "
                                                                                       "'Comment "
                + std::to_string(i) + "')"
            );
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Создаем тестовые документы (15 штук для тестов пагинации)
        for (int i = 1; i <= 15; ++i)
        {
            conn->execute(
                "INSERT INTO Document (caption, path, filename, size, uploadedAt, uploadedByUserId) "
                "VALUES ('Doc "
                + std::to_string(i) + "', '/tmp/doc" + std::to_string(i) + ".txt', "
                                                                           "'doc"
                + std::to_string(i) + ".txt', 1024, strftime('%s', 'now'), " + std::to_string(m_testUserId) + ")"
            );
        }

        m_repository = std::make_unique<repositories::SqliteCommentDocumentRepository>(m_database);

        // Сохраняем ID для использования в тестах
        auto stmt = conn->prepareStatement("SELECT id FROM Comment ORDER BY id");
        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            m_commentIds.push_back(rs->valueInt64(0));
        }

        stmt = conn->prepareStatement("SELECT id FROM Document ORDER BY id");
        rs = stmt->executeQuery();
        while (rs->next())
        {
            m_documentIds.push_back(rs->valueInt64(0));
        }
    }

    ~CommentDocumentRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteCommentDocumentRepository> m_repository;
    std::vector<int64_t> m_commentIds;
    std::vector<int64_t> m_documentIds;
    int64_t m_testUserId = 0;
    int64_t m_testItemId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteCommentDocumentRepositoryTests, CommentDocumentRepositoryFixture)

// ============================================================
// Тесты создания связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_comment_document_success)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 1);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    int64_t linkId = m_repository->create(link);
    BOOST_CHECK_GT(linkId, 0);

    auto found = m_repository->findById(linkId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->commentId, m_commentIds[0]);
    BOOST_CHECK_EQUAL(*found->documentId, m_documentIds[0]);
}

BOOST_AUTO_TEST_CASE(test_create_comment_document_duplicate_fails)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 1);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    int64_t linkId1 = m_repository->create(link);
    BOOST_CHECK_GT(linkId1, 0);

    int64_t linkId2 = m_repository->create(link);
    BOOST_CHECK_EQUAL(linkId2, 0);
}

BOOST_AUTO_TEST_CASE(test_create_comment_document_missing_fields)
{
    dto::CommentDocument link;
    link.commentId = 1;

    int64_t linkId = m_repository->create(link);
    BOOST_CHECK_EQUAL(linkId, 0);

    dto::CommentDocument link2;
    link2.documentId = 1;

    int64_t linkId2 = m_repository->create(link2);
    BOOST_CHECK_EQUAL(linkId2, 0);
}

// ============================================================
// Тесты поиска связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 1);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    int64_t linkId = m_repository->create(link);
    BOOST_REQUIRE_GT(linkId, 0);

    auto found = m_repository->findById(linkId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, linkId);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_comment_id)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 2);
    BOOST_REQUIRE_GE(m_documentIds.size(), 3);

    // Привязываем документы к commentId[0]
    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[0];
    link2.documentId = m_documentIds[1];
    m_repository->create(link2);

    // Привязываем документ к commentId[1]
    dto::CommentDocument link3;
    link3.commentId = m_commentIds[1];
    link3.documentId = m_documentIds[2];
    m_repository->create(link3);

    auto links = m_repository->findByCommentId(m_commentIds[0]);
    BOOST_CHECK_EQUAL(links.size(), 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.commentId, m_commentIds[0]);
    }

    auto links2 = m_repository->findByCommentId(m_commentIds[1]);
    BOOST_CHECK_EQUAL(links2.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_document_id)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 3);
    BOOST_REQUIRE_GE(m_documentIds.size(), 2);

    // Привязываем несколько комментариев к documentId[0]
    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[1];
    link2.documentId = m_documentIds[0];
    m_repository->create(link2);

    // Привязываем другой документ
    dto::CommentDocument link3;
    link3.commentId = m_commentIds[2];
    link3.documentId = m_documentIds[1];
    m_repository->create(link3);

    auto links = m_repository->findByDocumentId(m_documentIds[0]);
    BOOST_CHECK_EQUAL(links.size(), 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.documentId, m_documentIds[0]);
    }

    auto links2 = m_repository->findByDocumentId(m_documentIds[1]);
    BOOST_CHECK_EQUAL(links2.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_comment_and_document)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 2);
    BOOST_REQUIRE_GE(m_documentIds.size(), 2);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];
    m_repository->create(link);

    auto found = m_repository->findByCommentAndDocument(m_commentIds[0], m_documentIds[0]);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->commentId, m_commentIds[0]);
    BOOST_CHECK_EQUAL(*found->documentId, m_documentIds[0]);

    auto notFound = m_repository->findByCommentAndDocument(m_commentIds[0], m_documentIds[1]);
    BOOST_CHECK(!notFound.has_value());
}

// ============================================================
// Тесты findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [links, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(links.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Теперь у нас 15 комментариев и 15 документов, создаем 15 связей
    for (int i = 0; i < 15; ++i)
    {
        dto::CommentDocument link;
        link.commentId = m_commentIds[i];
        link.documentId = m_documentIds[i];
        m_repository->create(link);
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_comment)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 2);
    BOOST_REQUIRE_GE(m_documentIds.size(), 3);

    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[0];
    link2.documentId = m_documentIds[1];
    m_repository->create(link2);

    dto::CommentDocument link3;
    link3.commentId = m_commentIds[1];
    link3.documentId = m_documentIds[2];
    m_repository->create(link3);

    auto [links, total] = m_repository->findAll(1, 20, m_commentIds[0]);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.commentId, m_commentIds[0]);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_document)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 3);
    BOOST_REQUIRE_GE(m_documentIds.size(), 2);

    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[1];
    link2.documentId = m_documentIds[0];
    m_repository->create(link2);

    dto::CommentDocument link3;
    link3.commentId = m_commentIds[2];
    link3.documentId = m_documentIds[1];
    m_repository->create(link3);

    auto [links, total] = m_repository->findAll(1, 20, std::nullopt, m_documentIds[0]);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.documentId, m_documentIds[0]);
    }
}

// ============================================================
// Тесты удаления связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 1);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    int64_t linkId = m_repository->create(link);
    BOOST_REQUIRE_GT(linkId, 0);

    bool result = m_repository->remove(linkId);
    BOOST_CHECK(result);

    auto found = m_repository->findById(linkId);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_comment_id)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 2);
    BOOST_REQUIRE_GE(m_documentIds.size(), 3);

    // Создаем связи для commentId[0]
    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[0];
    link2.documentId = m_documentIds[1];
    m_repository->create(link2);

    // Создаем связь для commentId[1]
    dto::CommentDocument link3;
    link3.commentId = m_commentIds[1];
    link3.documentId = m_documentIds[2];
    m_repository->create(link3);

    int64_t removed = m_repository->removeByCommentId(m_commentIds[0]);
    BOOST_CHECK_EQUAL(removed, 2);

    auto links = m_repository->findByCommentId(m_commentIds[0]);
    BOOST_CHECK(links.empty());

    links = m_repository->findByCommentId(m_commentIds[1]);
    BOOST_CHECK_EQUAL(links.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_remove_by_document_id)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 3);
    BOOST_REQUIRE_GE(m_documentIds.size(), 2);

    // Создаем связи для documentId[0]
    dto::CommentDocument link1;
    link1.commentId = m_commentIds[0];
    link1.documentId = m_documentIds[0];
    m_repository->create(link1);

    dto::CommentDocument link2;
    link2.commentId = m_commentIds[1];
    link2.documentId = m_documentIds[0];
    m_repository->create(link2);

    // Создаем связь для documentId[1]
    dto::CommentDocument link3;
    link3.commentId = m_commentIds[2];
    link3.documentId = m_documentIds[1];
    m_repository->create(link3);

    int64_t removed = m_repository->removeByDocumentId(m_documentIds[0]);
    BOOST_CHECK_EQUAL(removed, 2);

    auto links = m_repository->findByDocumentId(m_documentIds[0]);
    BOOST_CHECK(links.empty());

    links = m_repository->findByDocumentId(m_documentIds[1]);
    BOOST_CHECK_EQUAL(links.size(), 1);
}

// ============================================================
// Тесты проверки существования связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 2);

    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    m_repository->create(link);

    BOOST_CHECK(m_repository->exists(m_commentIds[0], m_documentIds[0]));
    BOOST_CHECK(!m_repository->exists(m_commentIds[0], m_documentIds[1]));
    BOOST_CHECK(!m_repository->exists(m_commentIds[1], m_documentIds[0]));
}

// ============================================================
// Интеграционный тест: полный жизненный цикл связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_link_lifecycle)
{
    BOOST_REQUIRE_GE(m_commentIds.size(), 1);
    BOOST_REQUIRE_GE(m_documentIds.size(), 1);

    // 1. Создание
    dto::CommentDocument link;
    link.commentId = m_commentIds[0];
    link.documentId = m_documentIds[0];

    int64_t linkId = m_repository->create(link);
    BOOST_CHECK_GT(linkId, 0);

    // 2. Чтение
    auto found = m_repository->findById(linkId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->commentId, m_commentIds[0]);
    BOOST_CHECK_EQUAL(*found->documentId, m_documentIds[0]);

    // 3. Проверка существования
    BOOST_CHECK(m_repository->exists(m_commentIds[0], m_documentIds[0]));

    // 4. Удаление
    BOOST_CHECK(m_repository->remove(linkId));
    BOOST_CHECK(!m_repository->exists(m_commentIds[0], m_documentIds[0]));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
