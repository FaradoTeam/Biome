#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/board.h"

#include "repo/sqlite/sqlite_board_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct BoardRepositoryFixture
{
    BoardRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_board_repo.db";
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

        // Создаем тестовый Workflow
        conn->execute(
            "INSERT INTO Workflow (caption, description) "
            "VALUES ('Test Workflow', 'For testing boards')"
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

        // Создаем второй проект и фазу для тестов фильтрации
        conn->execute(R"(
            INSERT INTO Project (caption, createdAt, updatedAt)
            VALUES ('Second Project', strftime('%s', 'now'), strftime('%s', 'now'))
        )");
        m_secondProjectId = conn->lastInsertId();

        auto phaseStmt2 = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) "
            "VALUES (:projectId, 'Second Phase')"
        );
        phaseStmt2->bindInt64("projectId", m_secondProjectId);
        phaseStmt2->execute();
        m_secondPhaseId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteBoardRepository>(m_database);
    }

    dto::Board createTestBoard(
        const std::string& caption = "Test Board",
        std::optional<int64_t> projectId = std::nullopt,
        std::optional<int64_t> phaseId = std::nullopt,
        std::optional<int64_t> workflowId = std::nullopt
    )
    {
        dto::Board board;
        board.caption = caption;
        board.description = "Description of " + caption;
        board.workflowId = workflowId.has_value() ? *workflowId : m_testWorkflowId;
        board.projectId = projectId.has_value() ? *projectId : m_testProjectId;
        if (phaseId.has_value())
            board.phaseId = *phaseId;
        return board;
    }

    ~BoardRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteBoardRepository> m_repository;
    int64_t m_testWorkflowId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_secondProjectId = 0;
    int64_t m_secondPhaseId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteBoardRepositoryTests, BoardRepositoryFixture)

// ============================================================
// Тесты создания досок
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_board_success)
{
    auto board = createTestBoard("Новая доска");
    int64_t boardId = m_repository->create(board);

    BOOST_CHECK_GT(boardId, 0);
    BOOST_CHECK(m_repository->exists(boardId));

    auto found = m_repository->findById(boardId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новая доска");
    BOOST_CHECK_EQUAL(*found->workflowId, m_testWorkflowId);
    BOOST_CHECK_EQUAL(*found->projectId, m_testProjectId);
    BOOST_CHECK(!found->phaseId.has_value());
    BOOST_CHECK(found->description.has_value());
    BOOST_CHECK_EQUAL(*found->description, "Description of Новая доска");
}

BOOST_AUTO_TEST_CASE(test_create_board_with_phase)
{
    auto board = createTestBoard("Доска с фазой", m_testProjectId, m_testPhaseId);
    int64_t boardId = m_repository->create(board);

    BOOST_CHECK_GT(boardId, 0);

    auto found = m_repository->findById(boardId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->phaseId.has_value());
    BOOST_CHECK_EQUAL(*found->phaseId, m_testPhaseId);
}

BOOST_AUTO_TEST_CASE(test_create_board_missing_required_fields)
{
    dto::Board board;
    board.caption = "Board without workflow";
    board.projectId = m_testProjectId;

    int64_t boardId = m_repository->create(board);
    BOOST_CHECK_EQUAL(boardId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_board_empty_caption_fails)
{
    auto board = createTestBoard("");
    int64_t boardId = m_repository->create(board);
    BOOST_CHECK_EQUAL(boardId, 0);
}

// ============================================================
// Тесты поиска досок
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto board = createTestBoard("Доска для поиска");
    int64_t boardId = m_repository->create(board);
    BOOST_REQUIRE_GT(boardId, 0);

    auto found = m_repository->findById(boardId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, boardId);
    BOOST_CHECK_EQUAL(*found->caption, "Доска для поиска");
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
    auto [boards, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(boards.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Создаем 15 досок
    for (int i = 1; i <= 15; ++i)
    {
        m_repository->create(createTestBoard("Доска " + std::to_string(i)));
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_project)
{
    // Доски для первого проекта
    m_repository->create(createTestBoard("Проект 1 - Доска 1", m_testProjectId));
    m_repository->create(createTestBoard("Проект 1 - Доска 2", m_testProjectId));

    // Доски для второго проекта
    m_repository->create(createTestBoard("Проект 2 - Доска 1", m_secondProjectId));
    m_repository->create(createTestBoard("Проект 2 - Доска 2", m_secondProjectId));

    auto [boards, total] = m_repository->findAll(1, 20, m_testProjectId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& board : boards)
    {
        BOOST_CHECK_EQUAL(*board.projectId, m_testProjectId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_phase)
{
    // Доски с фазой
    m_repository->create(createTestBoard("Фаза 1 - Доска 1", m_testProjectId, m_testPhaseId));
    m_repository->create(createTestBoard("Фаза 1 - Доска 2", m_testProjectId, m_testPhaseId));

    // Доски без фазы
    m_repository->create(createTestBoard("Без фазы 1", m_testProjectId));

    auto [boards, total] = m_repository->findAll(1, 20, std::nullopt, m_testPhaseId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& board : boards)
    {
        BOOST_CHECK(board.phaseId.has_value());
        BOOST_CHECK_EQUAL(*board.phaseId, m_testPhaseId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_workflow)
{
    // Создаем второй Workflow
    auto conn = m_database->connection();
    conn->execute(
        "INSERT INTO Workflow (caption) VALUES ('Second Workflow')"
    );
    int64_t secondWorkflowId = conn->lastInsertId();

    m_repository->create(createTestBoard("Workflow 1 - Доска", m_testProjectId, std::nullopt, m_testWorkflowId));
    m_repository->create(createTestBoard("Workflow 2 - Доска", m_testProjectId, std::nullopt, secondWorkflowId));

    auto [boards, total] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, m_testWorkflowId);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*boards[0].workflowId, m_testWorkflowId);
}

// ============================================================
// Тесты обновления досок
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_board_success)
{
    auto board = createTestBoard("Старая доска");
    int64_t boardId = m_repository->create(board);
    BOOST_REQUIRE_GT(boardId, 0);

    dto::Board updateData;
    updateData.id = boardId;
    updateData.caption = "Новая доска";
    updateData.description = "Новое описание";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(boardId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новая доска");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");
    BOOST_CHECK_EQUAL(*found->workflowId, m_testWorkflowId);
    BOOST_CHECK_EQUAL(*found->projectId, m_testProjectId);
}

BOOST_AUTO_TEST_CASE(test_update_board_partial)
{
    auto board = createTestBoard("Оригинал");
    int64_t boardId = m_repository->create(board);

    dto::Board updateData;
    updateData.id = boardId;
    updateData.caption = "Обновленное название";

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(boardId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновленное название");
    BOOST_CHECK_EQUAL(*found->description, "Description of Оригинал");
}

BOOST_AUTO_TEST_CASE(test_update_board_add_phase)
{
    auto board = createTestBoard("Доска без фазы");
    int64_t boardId = m_repository->create(board);

    dto::Board updateData;
    updateData.id = boardId;
    updateData.phaseId = m_testPhaseId;

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(boardId);
    BOOST_CHECK(found->phaseId.has_value());
    BOOST_CHECK_EQUAL(*found->phaseId, m_testPhaseId);
}

BOOST_AUTO_TEST_CASE(test_update_board_nonexistent)
{
    dto::Board updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующая доска";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления досок
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_board_success)
{
    auto board = createTestBoard("Доска для удаления");
    int64_t boardId = m_repository->create(board);
    BOOST_REQUIRE_GT(boardId, 0);

    bool result = m_repository->remove(boardId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(boardId));
}

BOOST_AUTO_TEST_CASE(test_remove_board_nonexistent)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты поиска по проекту и фазе
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_project)
{
    m_repository->create(createTestBoard("Доска 1", m_testProjectId));
    m_repository->create(createTestBoard("Доска 2", m_testProjectId));
    m_repository->create(createTestBoard("Доска 3", m_secondProjectId));

    auto boards = m_repository->findByProject(m_testProjectId);
    BOOST_CHECK_EQUAL(boards.size(), 2);

    for (const auto& board : boards)
    {
        BOOST_CHECK_EQUAL(*board.projectId, m_testProjectId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_project_empty)
{
    auto boards = m_repository->findByProject(99999);
    BOOST_CHECK(boards.empty());
}

BOOST_AUTO_TEST_CASE(test_find_by_phase)
{
    m_repository->create(createTestBoard("Фазовая доска 1", m_testProjectId, m_testPhaseId));
    m_repository->create(createTestBoard("Фазовая доска 2", m_testProjectId, m_testPhaseId));
    m_repository->create(createTestBoard("Доска без фазы", m_testProjectId));

    auto boards = m_repository->findByPhase(m_testPhaseId);
    BOOST_CHECK_EQUAL(boards.size(), 2);

    for (const auto& board : boards)
    {
        BOOST_CHECK(board.phaseId.has_value());
        BOOST_CHECK_EQUAL(*board.phaseId, m_testPhaseId);
    }
}

// ============================================================
// Интеграционный тест: полный жизненный цикл доски
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_board_lifecycle)
{
    // 1. Создание
    auto board = createTestBoard("Жизненный цикл доски");
    int64_t boardId = m_repository->create(board);
    BOOST_CHECK_GT(boardId, 0);

    // 2. Чтение
    auto found = m_repository->findById(boardId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл доски");

    // 3. Обновление
    dto::Board updateData;
    updateData.id = boardId;
    updateData.caption = "Обновленная доска";
    updateData.description = "Новое описание";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(boardId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновленная доска");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");

    // 4. Проверка в списке
    auto [boards, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(boards[0].id.value(), boardId);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(boardId));
    BOOST_CHECK(!m_repository->exists(boardId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
