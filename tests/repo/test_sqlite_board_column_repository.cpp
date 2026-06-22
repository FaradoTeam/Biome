#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/board_column.h"

#include "repo/sqlite/sqlite_board_column_repository.h"
#include "repo/sqlite/sqlite_board_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct BoardColumnRepositoryFixture
{
    BoardColumnRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_board_column_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему
        conn->execute(R"(
            CREATE TABLE Workflow (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT
            )
        )");

        conn->execute(R"(
            CREATE TABLE Project (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT,
                searchCaption TEXT,
                searchDescription TEXT,
                createdAt INTEGER NOT NULL,
                updatedAt INTEGER NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Phase (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                projectId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                description TEXT,
                beginDate INTEGER,
                endDate INTEGER,
                isArchive INTEGER NOT NULL DEFAULT 0,
                searchCaption TEXT,
                searchDescription TEXT,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE State (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                description TEXT,
                weight INTEGER DEFAULT 0,
                orderNumber INTEGER DEFAULT 0,
                isArchive INTEGER NOT NULL DEFAULT 0,
                isQueue INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE Board (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                projectId INTEGER NOT NULL,
                phaseId INTEGER,
                caption TEXT NOT NULL,
                description TEXT,
                FOREIGN KEY (workflowId) REFERENCES Workflow(id) ON DELETE CASCADE,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE BoardColumn (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                boardId INTEGER NOT NULL,
                stateId INTEGER NOT NULL,
                orderNumber INTEGER DEFAULT 0,
                settings TEXT,
                FOREIGN KEY (boardId) REFERENCES Board(id) ON DELETE CASCADE,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                UNIQUE(boardId, stateId)
            )
        )");

        // Создаем тестовый Workflow
        conn->execute(
            "INSERT INTO Workflow (caption, description) "
            "VALUES ('Test Workflow', 'For testing board columns')"
        );
        m_testWorkflowId = conn->lastInsertId();

        // Создаем тестовый Project
        conn->execute(R"(
            INSERT INTO Project (caption, createdAt, updatedAt)
            VALUES ('Test Project', strftime('%s', 'now'), strftime('%s', 'now'))
        )");
        m_testProjectId = conn->lastInsertId();

        // Создаем тестовую Phase
        auto phaseStmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) "
            "VALUES (:projectId, 'Test Phase')"
        );
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_testPhaseId = conn->lastInsertId();

        // Создаем тестовые состояния
        std::vector<std::pair<std::string, int>> states = {
            { "Новая", 1 },
            { "В работе", 2 },
            { "На проверке", 3 },
            { "Завершена", 4 }
        };

        for (const auto& [caption, order] : states)
        {
            auto stmt = conn->prepareStatement(
                "INSERT INTO State (workflowId, caption, orderNumber) "
                "VALUES (:workflowId, :caption, :orderNumber)"
            );
            stmt->bindInt64("workflowId", m_testWorkflowId);
            stmt->bindString("caption", caption);
            stmt->bindInt64("orderNumber", order);
            stmt->execute();
            int64_t stateId = conn->lastInsertId();
            m_testStateIds.push_back(stateId);
        }

        // Создаем тестовую доску
        auto boardStmt = conn->prepareStatement(
            "INSERT INTO Board (workflowId, projectId, phaseId, caption) "
            "VALUES (:workflowId, :projectId, :phaseId, 'Test Board')"
        );
        boardStmt->bindInt64("workflowId", m_testWorkflowId);
        boardStmt->bindInt64("projectId", m_testProjectId);
        boardStmt->bindInt64("phaseId", m_testPhaseId);
        boardStmt->execute();
        m_testBoardId = conn->lastInsertId();

        // Создаем вторую доску для тестов фильтрации
        boardStmt->reset();
        boardStmt = conn->prepareStatement(
            "INSERT INTO Board (workflowId, projectId, phaseId, caption) "
            "VALUES (:workflowId, :projectId, :phaseId, 'Second Board')"
        );
        boardStmt->bindInt64("workflowId", m_testWorkflowId);
        boardStmt->bindInt64("projectId", m_testProjectId);
        boardStmt->bindInt64("phaseId", m_testPhaseId);
        boardStmt->execute();
        m_secondBoardId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteBoardColumnRepository>(m_database);
    }

    dto::BoardColumn createTestColumn(
        int64_t boardId = -1,
        int64_t stateId = -1,
        int orderNumber = 1,
        const std::string& settings = ""
    )
    {
        dto::BoardColumn column;
        column.boardId = (boardId > 0) ? boardId : m_testBoardId;
        column.stateId = (stateId > 0) ? stateId : m_testStateIds[0];
        column.orderNumber = orderNumber;
        if (!settings.empty())
            column.settings = settings;
        return column;
    }

    ~BoardColumnRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteBoardColumnRepository> m_repository;
    int64_t m_testWorkflowId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_testBoardId = 0;
    int64_t m_secondBoardId = 0;
    std::vector<int64_t> m_testStateIds;
};

BOOST_FIXTURE_TEST_SUITE(SqliteBoardColumnRepositoryTests, BoardColumnRepositoryFixture)

// ============================================================
// Тесты создания колонок
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_column_success)
{
    auto column = createTestColumn();
    int64_t columnId = m_repository->create(column);

    BOOST_CHECK_GT(columnId, 0);
    BOOST_CHECK(m_repository->exists(columnId));

    auto found = m_repository->findById(columnId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->boardId, m_testBoardId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateIds[0]);
    BOOST_CHECK_EQUAL(*found->orderNumber, 1);
    BOOST_CHECK(!found->settings.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_column_with_settings)
{
    auto column = createTestColumn(m_testBoardId, m_testStateIds[1], 2, R"({"wip": 5, "color": "green"})");
    int64_t columnId = m_repository->create(column);

    auto found = m_repository->findById(columnId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->settings.has_value());
    BOOST_CHECK_EQUAL(*found->settings, R"({"wip": 5, "color": "green"})");
}

BOOST_AUTO_TEST_CASE(test_create_column_duplicate_fails)
{
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 1));

    // Попытка создать колонку с той же парой (boardId, stateId)
    int64_t columnId = m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 2));
    BOOST_CHECK_EQUAL(columnId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_column_missing_required_fields)
{
    dto::BoardColumn column;
    column.stateId = m_testStateIds[0];

    int64_t columnId = m_repository->create(column);
    BOOST_CHECK_EQUAL(columnId, 0);
}

// ============================================================
// Тесты поиска колонок
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto column = createTestColumn();
    int64_t columnId = m_repository->create(column);
    BOOST_REQUIRE_GT(columnId, 0);

    auto found = m_repository->findById(columnId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, columnId);
    BOOST_CHECK_EQUAL(*found->boardId, m_testBoardId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateIds[0]);
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
    auto [columns, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(columns.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 12 колонок для разных состояний
    for (size_t i = 0; i < m_testStateIds.size() && i < 4; ++i)
    {
        m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[i], i + 1));
    }

    // Создаем колонки для второй доски
    for (size_t i = 0; i < m_testStateIds.size() && i < 4; ++i)
    {
        m_repository->create(createTestColumn(m_secondBoardId, m_testStateIds[i], i + 1));
    }

    auto [page1, total] = m_repository->findAll(1, 5);
    BOOST_CHECK_EQUAL(total, 8);
    BOOST_CHECK_EQUAL(page1.size(), 5);

    auto [page2, total2] = m_repository->findAll(2, 5);
    BOOST_CHECK_EQUAL(page2.size(), 3);
    BOOST_CHECK_EQUAL(total2, 8);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_board)
{
    // Колонки для первой доски
    for (size_t i = 0; i < 3; ++i)
    {
        m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[i], i + 1));
    }

    // Колонки для второй доски
    for (size_t i = 0; i < 2; ++i)
    {
        m_repository->create(createTestColumn(m_secondBoardId, m_testStateIds[i], i + 1));
    }

    auto [columns, total] = m_repository->findAll(1, 20, m_testBoardId);
    BOOST_CHECK_EQUAL(total, 3);
    for (const auto& col : columns)
    {
        BOOST_CHECK_EQUAL(*col.boardId, m_testBoardId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_state)
{
    int64_t stateId = m_testStateIds[1];

    // Создаем колонки с разными состояниями на разных досках
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 1));
    m_repository->create(createTestColumn(m_testBoardId, stateId, 2));
    m_repository->create(createTestColumn(m_secondBoardId, stateId, 1));

    auto [columns, total] = m_repository->findAll(1, 20, std::nullopt, stateId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& col : columns)
    {
        BOOST_CHECK_EQUAL(*col.stateId, stateId);
    }
}

// ============================================================
// Тесты обновления колонок
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_column_success)
{
    auto column = createTestColumn(m_testBoardId, m_testStateIds[0], 1);
    int64_t columnId = m_repository->create(column);
    BOOST_REQUIRE_GT(columnId, 0);

    dto::BoardColumn updateData;
    updateData.id = columnId;
    updateData.orderNumber = 5;
    updateData.settings = R"({"wip": 10})";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(columnId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->orderNumber, 5);
    BOOST_CHECK_EQUAL(*found->settings, R"({"wip": 10})");
    BOOST_CHECK_EQUAL(*found->boardId, m_testBoardId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateIds[0]);
}

BOOST_AUTO_TEST_CASE(test_update_column_partial)
{
    auto column = createTestColumn(m_testBoardId, m_testStateIds[0], 1);
    int64_t columnId = m_repository->create(column);

    dto::BoardColumn updateData;
    updateData.id = columnId;
    updateData.settings = R"({"color": "blue"})";

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(columnId);
    BOOST_CHECK_EQUAL(*found->settings, R"({"color": "blue"})");
    BOOST_CHECK_EQUAL(*found->orderNumber, 1);
}

BOOST_AUTO_TEST_CASE(test_update_column_change_state)
{
    auto column = createTestColumn(m_testBoardId, m_testStateIds[0], 1);
    int64_t columnId = m_repository->create(column);

    dto::BoardColumn updateData;
    updateData.id = columnId;
    updateData.stateId = m_testStateIds[1];

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(columnId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateIds[1]);
}

BOOST_AUTO_TEST_CASE(test_update_column_duplicate_state_fails)
{
    // Создаем две колонки с разными состояниями
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 1));
    int64_t columnId = m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[1], 2));
    BOOST_REQUIRE_GT(columnId, 0);

    // Пытаемся обновить вторую колонку на состояние, которое уже используется
    dto::BoardColumn updateData;
    updateData.id = columnId;
    updateData.stateId = m_testStateIds[0];

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);

    // Проверяем, что состояние не изменилось
    auto found = m_repository->findById(columnId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateIds[1]);
}

BOOST_AUTO_TEST_CASE(test_update_column_nonexistent)
{
    dto::BoardColumn updateData;
    updateData.id = 99999;
    updateData.orderNumber = 10;

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления колонок
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_column_success)
{
    auto column = createTestColumn();
    int64_t columnId = m_repository->create(column);
    BOOST_REQUIRE_GT(columnId, 0);

    bool result = m_repository->remove(columnId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(columnId));
}

BOOST_AUTO_TEST_CASE(test_remove_column_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_board_id)
{
    // Создаем колонки для первой доски
    for (size_t i = 0; i < 3; ++i)
    {
        m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[i], i + 1));
    }

    // Создаем колонки для второй доски
    for (size_t i = 0; i < 2; ++i)
    {
        m_repository->create(createTestColumn(m_secondBoardId, m_testStateIds[i], i + 1));
    }

    int64_t deletedCount = m_repository->removeByBoardId(m_testBoardId);
    BOOST_CHECK_EQUAL(deletedCount, 3);

    auto columns = m_repository->findByBoardId(m_testBoardId);
    BOOST_CHECK(columns.empty());

    // Колонки второй доски должны остаться
    auto secondColumns = m_repository->findByBoardId(m_secondBoardId);
    BOOST_CHECK_EQUAL(secondColumns.size(), 2);
}

// ============================================================
// Тесты проверки существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    auto column = createTestColumn();
    int64_t columnId = m_repository->create(column);
    BOOST_CHECK(m_repository->exists(columnId));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    BOOST_CHECK(!m_repository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_exists_by_board_and_state_true)
{
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 1));
    BOOST_CHECK(m_repository->existsByBoardAndState(m_testBoardId, m_testStateIds[0]));
}

BOOST_AUTO_TEST_CASE(test_exists_by_board_and_state_false)
{
    BOOST_CHECK(!m_repository->existsByBoardAndState(m_testBoardId, 99999));
}

// ============================================================
// Тесты поиска по доске и состоянию
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_board_id)
{
    // Создаем колонки для первой доски
    for (size_t i = 0; i < 3; ++i)
    {
        m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[i], i + 1));
    }

    // Создаем колонки для второй доски
    for (size_t i = 0; i < 2; ++i)
    {
        m_repository->create(createTestColumn(m_secondBoardId, m_testStateIds[i], i + 1));
    }

    auto columns = m_repository->findByBoardId(m_testBoardId);
    BOOST_CHECK_EQUAL(columns.size(), 3);

    // Проверяем сортировку по orderNumber
    for (size_t i = 0; i < columns.size(); ++i)
    {
        BOOST_CHECK_EQUAL(*columns[i].orderNumber, i + 1);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_board_id_no_order)
{
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 3));
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[1], 1));
    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[2], 2));

    auto columns = m_repository->findByBoardId(m_testBoardId, false);
    BOOST_CHECK_EQUAL(columns.size(), 3);
    // Без сортировки - порядок по ID
}

BOOST_AUTO_TEST_CASE(test_find_by_state_id)
{
    int64_t stateId = m_testStateIds[1];

    m_repository->create(createTestColumn(m_testBoardId, m_testStateIds[0], 1));
    m_repository->create(createTestColumn(m_testBoardId, stateId, 2));
    m_repository->create(createTestColumn(m_secondBoardId, stateId, 1));

    auto columns = m_repository->findByStateId(stateId);
    BOOST_CHECK_EQUAL(columns.size(), 2);

    for (const auto& col : columns)
    {
        BOOST_CHECK_EQUAL(*col.stateId, stateId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_board_id_empty)
{
    auto columns = m_repository->findByBoardId(99999);
    BOOST_CHECK(columns.empty());
}

// ============================================================
// Интеграционный тест: полный жизненный цикл колонки
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_column_lifecycle)
{
    // 1. Создание
    auto column = createTestColumn(m_testBoardId, m_testStateIds[0], 1, R"({"wip": 5})");
    int64_t columnId = m_repository->create(column);
    BOOST_CHECK_GT(columnId, 0);

    // 2. Чтение
    auto found = m_repository->findById(columnId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->orderNumber, 1);
    BOOST_CHECK_EQUAL(*found->settings, R"({"wip": 5})");

    // 3. Обновление
    dto::BoardColumn updateData;
    updateData.id = columnId;
    updateData.orderNumber = 3;
    updateData.settings = R"({"wip": 10, "color": "red"})";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(columnId);
    BOOST_CHECK_EQUAL(*found->orderNumber, 3);
    BOOST_CHECK_EQUAL(*found->settings, R"({"wip": 10, "color": "red"})");

    // 4. Проверка в списке
    auto [columns, total] = m_repository->findAll(1, 20, m_testBoardId);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(columns[0].id.value(), columnId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(columnId));
    BOOST_CHECK(!m_repository->exists(columnId));
}

// ============================================================
// Тест каскадного удаления
// ============================================================

BOOST_AUTO_TEST_CASE(test_cascade_delete_when_board_deleted)
{
    // Создаем колонку
    auto column = createTestColumn(m_testBoardId, m_testStateIds[0], 1);
    int64_t columnId = m_repository->create(column);
    BOOST_CHECK_GT(columnId, 0);

    // Удаляем доску
    auto boardRepo = std::make_unique<repositories::SqliteBoardRepository>(m_database);
    BOOST_CHECK(boardRepo->remove(m_testBoardId));

    // Колонка должна быть удалена каскадно
    BOOST_CHECK(!m_repository->exists(columnId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
