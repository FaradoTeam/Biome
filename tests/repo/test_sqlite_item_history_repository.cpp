#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/item_history.h"

#include "repo/sqlite/sqlite_item_history_repository.h"

#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct ItemHistoryRepositoryFixture
{
    ItemHistoryRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_item_history_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаём схему
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE Workflow (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE State (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                orderNumber INTEGER DEFAULT 0,
                isArchive INTEGER NOT NULL DEFAULT 0,
                isQueue INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE ItemType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                defaultStateId INTEGER,
                caption TEXT NOT NULL,
                kind TEXT NOT NULL,
                FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE Phase (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                projectId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE Project (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER NOT NULL,
                parentId INTEGER,
                stateId INTEGER NOT NULL,
                phaseId INTEGER,
                caption TEXT NOT NULL,
                content TEXT,
                isDeleted INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                FOREIGN KEY (parentId) REFERENCES Item(id) ON DELETE SET NULL,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE ItemHistory (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemId INTEGER NOT NULL,
                userId INTEGER NOT NULL,
                diff TEXT,
                timestamp INTEGER NOT NULL,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE
            )
        )");

        // Создаём тестового пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('testuser', 'test@example.com', 'hash')"
        );
        m_testUserId = conn->lastInsertId();

        // Создаём тестовый Workflow
        conn->execute("INSERT INTO Workflow (caption) VALUES ('Test Workflow')");
        m_testWorkflowId = conn->lastInsertId();

        // Создаём состояние
        auto stmt = conn->prepareStatement(
            "INSERT INTO State (workflowId, caption, orderNumber) "
            "VALUES (:workflowId, 'Новая', 1)"
        );
        stmt->bindInt64("workflowId", m_testWorkflowId);
        stmt->execute();
        m_testStateId = conn->lastInsertId();

        // Создаём тестовый ItemType
        auto itStmt = conn->prepareStatement(
            "INSERT INTO ItemType (workflowId, defaultStateId, caption, kind) "
            "VALUES (:workflowId, :defaultStateId, 'Задача', 'issue')"
        );
        itStmt->bindInt64("workflowId", m_testWorkflowId);
        itStmt->bindInt64("defaultStateId", m_testStateId);
        itStmt->execute();
        m_testItemTypeId = conn->lastInsertId();

        // Создаём тестовый Project
        conn->execute("INSERT INTO Project (caption) VALUES ('Test Project')");
        m_testProjectId = conn->lastInsertId();

        // Создаём тестовую Phase
        auto phaseStmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) VALUES (:projectId, 'Test Phase')"
        );
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_testPhaseId = conn->lastInsertId();

        // Создаём тестовый Item
        auto itemStmt = conn->prepareStatement(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) "
            "VALUES (:itemTypeId, :stateId, :phaseId, 'Тестовый элемент')"
        );
        itemStmt->bindInt64("itemTypeId", m_testItemTypeId);
        itemStmt->bindInt64("stateId", m_testStateId);
        itemStmt->bindInt64("phaseId", m_testPhaseId);
        itemStmt->execute();
        m_testItemId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteItemHistoryRepository>(m_database);
    }

    void clearTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM ItemHistory");
    }

    dto::ItemHistory createTestHistory(
        int64_t itemId,
        int64_t userId,
        const std::string& diff = ""
    )
    {
        dto::ItemHistory history;
        history.itemId = itemId;
        history.userId = userId;
        if (!diff.empty())
            history.diff = diff;
        history.timestamp = std::chrono::system_clock::now();
        return history;
    }

    ~ItemHistoryRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteItemHistoryRepository> m_repository;
    int64_t m_testUserId = 0;
    int64_t m_testWorkflowId = 0;
    int64_t m_testItemTypeId = 0;
    int64_t m_testStateId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_testItemId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteItemHistoryRepositoryTests, ItemHistoryRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_success)
{
    clearTable();
    auto history = createTestHistory(m_testItemId, m_testUserId, R"({"test": "data"})");
    int64_t id = m_repository->create(history);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_missing_required_fields)
{
    clearTable();
    dto::ItemHistory history;
    history.itemId = m_testItemId;
    int64_t id = m_repository->create(history);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearTable();
    auto history = createTestHistory(m_testItemId, m_testUserId);
    int64_t id = m_repository->create(history);
    BOOST_REQUIRE_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemId);
    BOOST_CHECK_EQUAL(*found->userId, m_testUserId);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearTable();
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id)
{
    clearTable();
    m_repository->create(createTestHistory(m_testItemId, m_testUserId));
    m_repository->create(createTestHistory(m_testItemId, m_testUserId, R"({"update": true})"));

    auto histories = m_repository->findByItemId(m_testItemId);
    BOOST_CHECK_EQUAL(histories.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    clearTable();
    m_repository->create(createTestHistory(m_testItemId, m_testUserId));

    auto histories = m_repository->findByUserId(m_testUserId);
    BOOST_CHECK_EQUAL(histories.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_last_by_item_id)
{
    clearTable();

    // Создаём первую запись (более старую)
    auto history1 = createTestHistory(m_testItemId, m_testUserId);
    history1.timestamp = std::chrono::system_clock::now() - std::chrono::seconds(10);
    m_repository->create(history1);

    // Создаём вторую запись с diff (более новую)
    auto history2 = createTestHistory(m_testItemId, m_testUserId, R"({"stateId": 3})");
    history2.timestamp = std::chrono::system_clock::now();
    int64_t id2 = m_repository->create(history2);

    auto last = m_repository->findLastByItemId(m_testItemId);
    BOOST_REQUIRE(last.has_value());
    BOOST_CHECK_EQUAL(*last->id, id2);
    BOOST_CHECK(last->diff.has_value());
    BOOST_CHECK_EQUAL(*last->diff, R"({"stateId": 3})");
}

BOOST_AUTO_TEST_CASE(test_find_last_by_item_id_empty)
{
    clearTable();
    auto last = m_repository->findLastByItemId(99999);
    BOOST_CHECK(!last.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearTable();

    for (int i = 0; i < 3; ++i)
    {
        m_repository->create(createTestHistory(m_testItemId, m_testUserId));
    }

    auto [page1, total] = m_repository->findAll(1, 2);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(page1.size(), 2);

    auto [page2, total2] = m_repository->findAll(2, 2);
    BOOST_CHECK_EQUAL(total2, 3);
    BOOST_CHECK_EQUAL(page2.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_item_id)
{
    clearTable();
    m_repository->create(createTestHistory(m_testItemId, m_testUserId));
    m_repository->create(createTestHistory(m_testItemId, m_testUserId, R"({"update": true})"));

    auto [histories, total] = m_repository->findAll(1, 20, m_testItemId);
    BOOST_CHECK_EQUAL(total, 2);
    BOOST_CHECK_EQUAL(histories.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_date_range)
{
    clearTable();

    auto now = std::chrono::system_clock::now();
    auto yesterday = now - std::chrono::hours(24);
    auto tomorrow = now + std::chrono::hours(24);

    dto::ItemHistory history;
    history.itemId = m_testItemId;
    history.userId = m_testUserId;
    history.timestamp = now;
    m_repository->create(history);

    auto [histories, total] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, yesterday, tomorrow);
    BOOST_CHECK_EQUAL(total, 1);
}

BOOST_AUTO_TEST_CASE(test_update)
{
    clearTable();
    auto history = createTestHistory(m_testItemId, m_testUserId, R"({"old": "value"})");
    int64_t id = m_repository->create(history);
    BOOST_REQUIRE_GT(id, 0);

    dto::ItemHistory updateData;
    updateData.id = id;
    updateData.diff = R"({"new": "value"})";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->diff, R"({"new": "value"})");
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemId);
    BOOST_CHECK_EQUAL(*found->userId, m_testUserId);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent)
{
    clearTable();
    dto::ItemHistory updateData;
    updateData.id = 99999;
    updateData.diff = R"({"test": "value"})";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    clearTable();
    auto history = createTestHistory(m_testItemId, m_testUserId);
    int64_t id = m_repository->create(history);
    BOOST_REQUIRE_GT(id, 0);

    bool result = m_repository->remove(id);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent)
{
    clearTable();
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id)
{
    clearTable();
    m_repository->create(createTestHistory(m_testItemId, m_testUserId));
    m_repository->create(createTestHistory(m_testItemId, m_testUserId, R"({"update": true})"));

    int64_t deletedCount = m_repository->removeByItemId(m_testItemId);
    BOOST_CHECK_EQUAL(deletedCount, 2);

    auto histories = m_repository->findByItemId(m_testItemId);
    BOOST_CHECK(histories.empty());
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id_nonexistent)
{
    clearTable();
    int64_t deletedCount = m_repository->removeByItemId(99999);
    BOOST_CHECK_EQUAL(deletedCount, 0);
}

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearTable();
    auto history = createTestHistory(m_testItemId, m_testUserId);
    int64_t id = m_repository->create(history);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    clearTable();
    BOOST_CHECK(!m_repository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_full_lifecycle)
{
    clearTable();

    // 1. Создание
    auto history = createTestHistory(m_testItemId, m_testUserId, R"({"caption": "Жизненный цикл"})");
    int64_t id = m_repository->create(history);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->diff, R"({"caption": "Жизненный цикл"})");

    // 3. Обновление
    dto::ItemHistory updateData;
    updateData.id = id;
    updateData.diff = R"({"caption": "Обновлённый жизненный цикл"})";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->diff, R"({"caption": "Обновлённый жизненный цикл"})");

    // 4. Проверка в списке
    auto [histories, total] = m_repository->findAll(1, 20, m_testItemId);
    BOOST_CHECK_EQUAL(total, 1);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
