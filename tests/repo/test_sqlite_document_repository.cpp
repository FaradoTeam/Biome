#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/document.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_document_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct DocumentRepositoryFixture
{
    DocumentRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_document_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для документов
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
            CREATE TABLE Document (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT,
                path TEXT NOT NULL UNIQUE,
                filename TEXT NOT NULL,
                size INTEGER NOT NULL,
                mimeType TEXT,
                uploadedAt INTEGER NOT NULL,
                uploadedByUserId INTEGER NOT NULL,
                searchCaption TEXT,
                searchDescription TEXT,
                FOREIGN KEY (uploadedByUserId) REFERENCES User(id) ON DELETE CASCADE
            )
        )");

        // Создаем тестового пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('test_user', 'test@mail.local', 'hash')"
        );
        m_testUserId = conn->lastInsertId();

        // Создаем второго пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('test_user2', 'test2@mail.local', 'hash')"
        );
        m_secondUserId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteDocumentRepository>(m_database);
    }

    dto::Document createTestDocument(
        const std::string& caption = "Test Document",
        const std::string& path = "/tmp/test_doc.txt",
        const std::string& filename = "test_doc.txt",
        int64_t size = 1024,
        const std::string& mimeType = "text/plain",
        std::optional<int64_t> uploadedByUserId = std::nullopt
    )
    {
        dto::Document doc;
        doc.caption = caption;
        doc.description = "Description of " + caption;
        doc.path = path;
        doc.filename = filename;
        doc.size = size;
        doc.mimeType = mimeType;
        doc.uploadedByUserId = uploadedByUserId.has_value() ? *uploadedByUserId : m_testUserId;
        doc.uploadedAt = std::chrono::system_clock::now();
        return doc;
    }

    ~DocumentRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteDocumentRepository> m_repository;
    int64_t m_testUserId = 0;
    int64_t m_secondUserId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteDocumentRepositoryTests, DocumentRepositoryFixture)

// ============================================================
// Тесты создания документов
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_document_success)
{
    auto doc = createTestDocument("Новый документ", "/tmp/new_doc.pdf", "new_doc.pdf", 2048, "application/pdf");
    int64_t docId = m_repository->create(doc);

    BOOST_CHECK_GT(docId, 0);
    BOOST_CHECK(m_repository->exists(docId));

    auto found = m_repository->findById(docId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новый документ");
    BOOST_CHECK_EQUAL(*found->path, "/tmp/new_doc.pdf");
    BOOST_CHECK_EQUAL(*found->filename, "new_doc.pdf");
    BOOST_CHECK_EQUAL(*found->size, 2048);
    BOOST_CHECK_EQUAL(*found->mimeType, "application/pdf");
    BOOST_CHECK_EQUAL(*found->uploadedByUserId, m_testUserId);
    BOOST_CHECK(found->uploadedAt.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_document_without_mime_type)
{
    auto doc = createTestDocument("Документ без MIME", "/tmp/no_mime.bin", "no_mime.bin", 512);
    doc.mimeType = std::nullopt;

    int64_t docId = m_repository->create(doc);
    BOOST_CHECK_GT(docId, 0);

    auto found = m_repository->findById(docId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->mimeType.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_document_missing_required_fields)
{
    dto::Document doc;
    doc.caption = "Документ без пути";

    int64_t docId = m_repository->create(doc);
    BOOST_CHECK_EQUAL(docId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_document_empty_caption_fails)
{
    auto doc = createTestDocument("", "/tmp/empty_caption.txt", "empty_caption.txt", 100);
    int64_t docId = m_repository->create(doc);
    BOOST_CHECK_EQUAL(docId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_document_duplicate_path_fails)
{
    const std::string path = "/tmp/duplicate_path.txt";

    auto doc1 = createTestDocument("Документ 1", path, "file1.txt", 100);
    int64_t docId1 = m_repository->create(doc1);
    BOOST_CHECK_GT(docId1, 0);

    auto doc2 = createTestDocument("Документ 2", path, "file2.txt", 200);
    int64_t docId2 = m_repository->create(doc2);
    BOOST_CHECK_EQUAL(docId2, 0);
}

// ============================================================
// Тесты поиска документов
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto doc = createTestDocument("Документ для поиска", "/tmp/search_doc.txt", "search_doc.txt", 1024);
    int64_t docId = m_repository->create(doc);
    BOOST_REQUIRE_GT(docId, 0);

    auto found = m_repository->findById(docId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, docId);
    BOOST_CHECK_EQUAL(*found->caption, "Документ для поиска");
    BOOST_CHECK_EQUAL(*found->path, "/tmp/search_doc.txt");
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
    auto [docs, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(docs.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 15 документов
    for (int i = 1; i <= 15; ++i)
    {
        m_repository->create(
            createTestDocument(
                "Документ " + std::to_string(i),
                "/tmp/doc_" + std::to_string(i) + ".txt",
                "doc_" + std::to_string(i) + ".txt",
                1024 + i
            )
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    // Документы первого пользователя
    m_repository->create(createTestDocument("Пользователь 1 - Док 1", "/tmp/user1_1.txt", "user1_1.txt", 100, "text/plain", m_testUserId));
    m_repository->create(createTestDocument("Пользователь 1 - Док 2", "/tmp/user1_2.txt", "user1_2.txt", 200, "text/plain", m_testUserId));

    // Документы второго пользователя
    m_repository->create(createTestDocument("Пользователь 2 - Док 1", "/tmp/user2_1.txt", "user2_1.txt", 300, "text/plain", m_secondUserId));
    m_repository->create(createTestDocument("Пользователь 2 - Док 2", "/tmp/user2_2.txt", "user2_2.txt", 400, "text/plain", m_secondUserId));

    auto [docs, total] = m_repository->findAll(1, 20, m_testUserId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& doc : docs)
    {
        BOOST_CHECK_EQUAL(*doc.uploadedByUserId, m_testUserId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_search_by_caption)
{
    m_repository->create(createTestDocument("Отчет по проекту Альфа", "/tmp/report_alpha.txt", "report_alpha.txt", 1000));
    m_repository->create(createTestDocument("Отчет по проекту Бета", "/tmp/report_beta.txt", "report_beta.txt", 2000));
    m_repository->create(createTestDocument("Презентация", "/tmp/presentation.pptx", "presentation.pptx", 5000));

    auto [docs, total] = m_repository->findAll(1, 20, std::nullopt, "Отчет");
    BOOST_CHECK_EQUAL(total, 2);

    for (const auto& doc : docs)
    {
        BOOST_CHECK(doc.caption->find("Отчет") != std::string::npos);
    }
}

// ============================================================
// Тесты обновления документов
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_document_success)
{
    auto doc = createTestDocument("Старый документ", "/tmp/old_doc.txt", "old_doc.txt", 1024);
    int64_t docId = m_repository->create(doc);
    BOOST_REQUIRE_GT(docId, 0);

    dto::Document updateData;
    updateData.id = docId;
    updateData.caption = "Новый документ";
    updateData.description = "Новое описание";
    updateData.path = "/tmp/new_doc.txt";
    updateData.filename = "new_doc.txt";
    updateData.size = 2048;
    updateData.mimeType = "application/pdf";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(docId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новый документ");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");
    BOOST_CHECK_EQUAL(*found->path, "/tmp/new_doc.txt");
    BOOST_CHECK_EQUAL(*found->filename, "new_doc.txt");
    BOOST_CHECK_EQUAL(*found->size, 2048);
    BOOST_CHECK_EQUAL(*found->mimeType, "application/pdf");
}

BOOST_AUTO_TEST_CASE(test_update_document_partial)
{
    auto doc = createTestDocument("Оригинал", "/tmp/original.txt", "original.txt", 1024);
    int64_t docId = m_repository->create(doc);

    dto::Document updateData;
    updateData.id = docId;
    updateData.caption = "Обновленное название";
    updateData.size = 2048;

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(docId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновленное название");
    BOOST_CHECK_EQUAL(*found->size, 2048);
    BOOST_CHECK_EQUAL(*found->path, "/tmp/original.txt");
    BOOST_CHECK_EQUAL(*found->filename, "original.txt");
}

BOOST_AUTO_TEST_CASE(test_update_document_duplicate_path_fails)
{
    // Создаем два документа с разными путями
    auto doc1 = createTestDocument("Документ 1", "/tmp/path1.txt", "path1.txt", 100);
    int64_t docId1 = m_repository->create(doc1);

    auto doc2 = createTestDocument("Документ 2", "/tmp/path2.txt", "path2.txt", 200);
    int64_t docId2 = m_repository->create(doc2);

    // Пытаемся обновить doc2, устанавливая путь doc1
    dto::Document updateData;
    updateData.id = docId2;
    updateData.path = "/tmp/path1.txt";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_document_nonexistent)
{
    dto::Document updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующий документ";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления документов
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_document_success)
{
    auto doc = createTestDocument("Документ для удаления", "/tmp/delete_me.txt", "delete_me.txt", 1024);
    int64_t docId = m_repository->create(doc);
    BOOST_REQUIRE_GT(docId, 0);

    bool result = m_repository->remove(docId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(docId));
}

BOOST_AUTO_TEST_CASE(test_remove_document_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты проверки уникальности пути
// ============================================================

BOOST_AUTO_TEST_CASE(test_is_path_unique_success)
{
    BOOST_CHECK(m_repository->isPathUnique("/tmp/unique_path.txt"));

    auto doc = createTestDocument("Документ", "/tmp/unique_path.txt", "file.txt", 100);
    m_repository->create(doc);

    BOOST_CHECK(!m_repository->isPathUnique("/tmp/unique_path.txt"));
}

BOOST_AUTO_TEST_CASE(test_is_path_unique_with_exclude)
{
    auto doc = createTestDocument("Документ", "/tmp/exclude_test.txt", "file.txt", 100);
    int64_t docId = m_repository->create(doc);

    // Путь занят, но исключаем текущий документ
    BOOST_CHECK(m_repository->isPathUnique("/tmp/exclude_test.txt", docId));

    // Исключаем другой ID
    BOOST_CHECK(!m_repository->isPathUnique("/tmp/exclude_test.txt", 99999));
}

// ============================================================
// Интеграционный тест: полный жизненный цикл документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_document_lifecycle)
{
    // 1. Создание
    auto doc = createTestDocument("Жизненный цикл документа", "/tmp/lifecycle.txt", "lifecycle.txt", 1024);
    int64_t docId = m_repository->create(doc);
    BOOST_CHECK_GT(docId, 0);

    // 2. Чтение
    auto found = m_repository->findById(docId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл документа");

    // 3. Обновление
    dto::Document updateData;
    updateData.id = docId;
    updateData.caption = "Обновленный документ";
    updateData.size = 2048;
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(docId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновленный документ");
    BOOST_CHECK_EQUAL(*found->size, 2048);

    // 4. Проверка в списке
    auto [docs, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(docs[0].id.value(), docId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(docId));
    BOOST_CHECK(!m_repository->exists(docId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
