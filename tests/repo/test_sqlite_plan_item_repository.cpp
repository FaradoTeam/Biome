#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/item.h"
#include "common/dto/item_type.h"
#include "common/dto/phase.h"
#include "common/dto/plan_item.h"
#include "common/dto/project.h"
#include "common/dto/state.h"
#include "common/dto/workflow.h"

#include "repo/sqlite/sqlite_item_repository.h"
#include "repo/sqlite/sqlite_item_type_repository.h"
#include "repo/sqlite/sqlite_phase_repository.h"
#include "repo/sqlite/sqlite_plan_item_repository.h"
#include "repo/sqlite/sqlite_plan_repository.h"
#include "repo/sqlite/sqlite_project_repository.h"
#include "repo/sqlite/sqlite_state_repository.h"
#include "repo/sqlite/sqlite_workflow_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct PlanItemRepositoryFixture
{
    PlanItemRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_plan_item_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Включаем поддержку внешних ключей
        conn->execute("PRAGMA foreign_keys=ON");

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
            CREATE TABLE Project (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0
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
            CREATE TABLE Item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER NOT NULL,
                parentId INTEGER,
                stateId INTEGER NOT NULL,
                phaseId INTEGER,
                caption TEXT NOT NULL,
                content TEXT,
                searchCaption TEXT,
                searchContent TEXT,
                isDeleted INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                FOREIGN KEY (parentId) REFERENCES Item(id) ON DELETE SET NULL,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE Plan (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                phaseId INTEGER NOT NULL,
                basePlanId INTEGER,
                caption TEXT NOT NULL,
                description TEXT,
                isActive INTEGER NOT NULL DEFAULT 0,
                createdAt INTEGER NOT NULL,
                createdByUserId INTEGER NOT NULL,
                activatedAt INTEGER,
                activatedByUserId INTEGER,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE CASCADE,
                FOREIGN KEY (basePlanId) REFERENCES Plan(id) ON DELETE SET NULL,
                FOREIGN KEY (createdByUserId) REFERENCES User(id),
                FOREIGN KEY (activatedByUserId) REFERENCES User(id)
            )
        )");

        conn->execute(R"(
            CREATE TABLE PlanItem (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemId INTEGER NOT NULL,
                userId INTEGER,
                planId INTEGER NOT NULL,
                startDate INTEGER NOT NULL,
                endDate INTEGER NOT NULL,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE SET NULL,
                FOREIGN KEY (planId) REFERENCES Plan(id) ON DELETE CASCADE,
                UNIQUE(planId, itemId)
            )
        )");

        // Создаём тестового пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('testuser', 'test@example.com', 'hash')"
        );
        m_testUserId = conn->lastInsertId();

        // Создаём второго пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('testuser2', 'test2@example.com', 'hash2')"
        );
        m_secondUserId = conn->lastInsertId();

        // Создаём тестовый Workflow
        conn->execute("INSERT INTO Workflow (caption) VALUES ('Test Workflow')");
        m_testWorkflowId = conn->lastInsertId();

        // Создаём состояние
        auto stateStmt = conn->prepareStatement(
            "INSERT INTO State (workflowId, caption, orderNumber) "
            "VALUES (:workflowId, 'Новая', 1)"
        );
        stateStmt->bindInt64("workflowId", m_testWorkflowId);
        stateStmt->execute();
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

        // Создаём тестовый проект
        conn->execute("INSERT INTO Project (caption) VALUES ('Test Project')");
        m_testProjectId = conn->lastInsertId();

        // Создаём тестовую фазу
        auto phaseStmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) VALUES (:projectId, 'Test Phase')"
        );
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_testPhaseId = conn->lastInsertId();

        // Создаём тестовые элементы
        auto itemStmt = conn->prepareStatement(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) "
            "VALUES (:itemTypeId, :stateId, :phaseId, :caption)"
        );

        for (int i = 1; i <= 3; ++i)
        {
            itemStmt->bindInt64("itemTypeId", m_testItemTypeId);
            itemStmt->bindInt64("stateId", m_testStateId);
            itemStmt->bindInt64("phaseId", m_testPhaseId);
            itemStmt->bindString("caption", "Тестовый элемент " + std::to_string(i));
            itemStmt->execute();
            m_testItemIds.push_back(conn->lastInsertId());
        }

        // Создаём планы
        auto planStmt = conn->prepareStatement(
            "INSERT INTO Plan (phaseId, caption, createdAt, createdByUserId) "
            "VALUES (:phaseId, :caption, :createdAt, :createdByUserId)"
        );
        auto now = common::timePointToSeconds(std::chrono::system_clock::now());

        planStmt->bindInt64("phaseId", m_testPhaseId);
        planStmt->bindString("caption", "Активный план");
        planStmt->bindInt64("createdAt", now);
        planStmt->bindInt64("createdByUserId", m_testUserId);
        planStmt->execute();
        m_activePlanId = conn->lastInsertId();

        // Делаем план активным
        auto activateStmt = conn->prepareStatement(
            "UPDATE Plan SET isActive = 1 WHERE id = :id"
        );
        activateStmt->bindInt64("id", m_activePlanId);
        activateStmt->execute();

        planStmt->reset();
        planStmt->bindInt64("phaseId", m_testPhaseId);
        planStmt->bindString("caption", "Черновик план");
        planStmt->bindInt64("createdAt", now);
        planStmt->bindInt64("createdByUserId", m_testUserId);
        planStmt->execute();
        m_draftPlanId = conn->lastInsertId();

        m_planItemRepository = std::make_unique<repositories::SqlitePlanItemRepository>(m_database);
        m_planRepository = std::make_unique<repositories::SqlitePlanRepository>(m_database);
    }

    void clearTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM PlanItem");
    }

    dto::PlanItem createTestPlanItem(
        int64_t planId,
        int64_t itemId,
        std::optional<int64_t> userId = std::nullopt,
        std::optional<int64_t> daysOffsetStart = std::nullopt,
        std::optional<int64_t> daysOffsetEnd = std::nullopt
    )
    {
        dto::PlanItem planItem;
        planItem.planId = planId;
        planItem.itemId = itemId;
        if (userId.has_value())
            planItem.userId = userId;

        auto now = std::chrono::system_clock::now();
        planItem.startDate = now + std::chrono::hours(24 * daysOffsetStart.value_or(1));
        planItem.endDate = now + std::chrono::hours(24 * daysOffsetEnd.value_or(5));
        return planItem;
    }

    ~PlanItemRepositoryFixture()
    {
        m_planItemRepository.reset();
        m_planRepository.reset();

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
    std::unique_ptr<repositories::SqlitePlanItemRepository> m_planItemRepository;
    std::unique_ptr<repositories::SqlitePlanRepository> m_planRepository;
    std::vector<int64_t> m_testItemIds;
    int64_t m_testUserId = 0;
    int64_t m_secondUserId = 0;
    int64_t m_testWorkflowId = 0;
    int64_t m_testItemTypeId = 0;
    int64_t m_testStateId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_activePlanId = 0;
    int64_t m_draftPlanId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqlitePlanItemRepositoryTests, PlanItemRepositoryFixture)

// ============================================================
// Тесты создания элементов плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_plan_item_success)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);

    BOOST_CHECK_GT(planItemId, 0);
    BOOST_CHECK(m_planItemRepository->exists(planItemId));

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->planId, m_activePlanId);
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemIds[0]);
    BOOST_CHECK(found->userId.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_testUserId);
    BOOST_CHECK(found->startDate.has_value());
    BOOST_CHECK(found->endDate.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_plan_item_without_user)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0]);
    planItem.userId = std::nullopt;
    int64_t planItemId = m_planItemRepository->create(planItem);

    BOOST_CHECK_GT(planItemId, 0);

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->userId.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_plan_item_duplicate_fails)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0]);
    int64_t firstId = m_planItemRepository->create(planItem);
    BOOST_CHECK_GT(firstId, 0);

    // Попытка создать дубликат (те же planId и itemId)
    int64_t secondId = m_planItemRepository->create(planItem);
    BOOST_CHECK_EQUAL(secondId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_plan_item_missing_required_fields)
{
    clearTable();

    dto::PlanItem planItem;
    planItem.planId = m_activePlanId;
    // Нет itemId, startDate, endDate

    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_CHECK_EQUAL(planItemId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_plan_item_invalid_plan)
{
    clearTable();

    auto planItem = createTestPlanItem(99999, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_CHECK_EQUAL(planItemId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_plan_item_invalid_item)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, 99999, m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_CHECK_EQUAL(planItemId, 0);
}

// ============================================================
// Тесты поиска элементов плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_REQUIRE_GT(planItemId, 0);

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, planItemId);
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemIds[0]);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearTable();

    auto found = m_planItemRepository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_plan_and_item_success)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);

    auto found = m_planItemRepository->findByPlanAndItem(m_activePlanId, m_testItemIds[0]);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, planItemId);
}

BOOST_AUTO_TEST_CASE(test_find_by_plan_and_item_not_found)
{
    clearTable();

    auto found = m_planItemRepository->findByPlanAndItem(m_activePlanId, 99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_plan_id)
{
    clearTable();

    // Создаём элементы для активного плана
    for (size_t i = 0; i < 3; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    // Создаём элемент для черновика
    auto draftItem = createTestPlanItem(m_draftPlanId, m_testItemIds[0], m_testUserId);
    m_planItemRepository->create(draftItem);

    auto items = m_planItemRepository->findByPlanId(m_activePlanId);
    BOOST_CHECK_EQUAL(items.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    clearTable();

    // Создаём элементы для первого пользователя
    for (size_t i = 0; i < 2; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    // Создаём элемент для второго пользователя
    auto user2Item = createTestPlanItem(m_activePlanId, m_testItemIds[2], m_secondUserId);
    m_planItemRepository->create(user2Item);

    auto items = m_planItemRepository->findByUserId(m_testUserId);
    BOOST_CHECK_EQUAL(items.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_by_date_range)
{
    clearTable();

    auto now = std::chrono::system_clock::now();

    // Элемент, начинающийся завтра и заканчивающийся через 3 дня
    auto item1 = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId, 1, 3);
    m_planItemRepository->create(item1);

    // Элемент, начинающийся через 5 дней и заканчивающийся через 7 дней
    auto item2 = createTestPlanItem(m_activePlanId, m_testItemIds[1], m_testUserId, 5, 7);
    m_planItemRepository->create(item2);

    // Элемент, начинающийся через 10 дней
    auto item3 = createTestPlanItem(m_activePlanId, m_testItemIds[2], m_testUserId, 10, 15);
    m_planItemRepository->create(item3);

    auto dateFrom = now + std::chrono::hours(24 * 2);
    auto dateTo = now + std::chrono::hours(24 * 6);

    auto items = m_planItemRepository->findByDateRange(dateFrom, dateTo);
    // Должны найтись item1 (пересекается) и item2 (внутри диапазона)
    BOOST_CHECK_EQUAL(items.size(), 2);
}

// ============================================================
// Тесты findAll с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    clearTable();

    auto [items, total] = m_planItemRepository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(items.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearTable();

    // Создаём 15 элементов плана
    for (int i = 0; i < 15; ++i)
    {
        auto planItem = createTestPlanItem(
            m_activePlanId,
            m_testItemIds[i % m_testItemIds.size()],
            m_testUserId,
            i + 1,
            i + 5
        );
        m_planItemRepository->create(planItem);
    }

    auto [page1, total] = m_planItemRepository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_planItemRepository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_plan_id)
{
    clearTable();

    // Создаём элементы для активного плана
    for (size_t i = 0; i < 3; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    // Создаём элемент для черновика
    auto draftItem = createTestPlanItem(m_draftPlanId, m_testItemIds[0], m_testUserId);
    m_planItemRepository->create(draftItem);

    auto [items, total] = m_planItemRepository->findAll(1, 20, m_activePlanId);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(items.size(), 3);

    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(*item.planId, m_activePlanId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user_id)
{
    clearTable();

    // Создаём элементы для первого пользователя
    for (size_t i = 0; i < 3; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    // Создаём элемент для второго пользователя
    auto user2Item = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_secondUserId);
    m_planItemRepository->create(user2Item);

    auto [items, total] = m_planItemRepository->findAll(1, 20, std::nullopt, m_testUserId);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(items.size(), 3);

    for (const auto& item : items)
    {
        BOOST_CHECK(item.userId.has_value());
        BOOST_CHECK_EQUAL(*item.userId, m_testUserId);
    }
}

// ============================================================
// Тесты обновления элементов плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_plan_item_success)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_REQUIRE_GT(planItemId, 0);

    auto newStartDate = std::chrono::system_clock::now() + std::chrono::hours(24 * 10);
    auto newEndDate = std::chrono::system_clock::now() + std::chrono::hours(24 * 20);

    dto::PlanItem updateData;
    updateData.id = planItemId;
    updateData.userId = m_secondUserId;
    updateData.startDate = newStartDate;
    updateData.endDate = newEndDate;

    bool result = m_planItemRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->userId.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_secondUserId);
    BOOST_CHECK(found->startDate.has_value());
    BOOST_CHECK(found->endDate.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_partial)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_REQUIRE_GT(planItemId, 0);

    auto originalStartDate = *planItem.startDate;
    auto originalEndDate = *planItem.endDate;

    dto::PlanItem updateData;
    updateData.id = planItemId;
    updateData.userId = m_secondUserId;

    bool result = m_planItemRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_secondUserId);
    // Даты не должны измениться
    BOOST_CHECK(found->startDate.has_value());
    BOOST_CHECK(found->endDate.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_nonexistent)
{
    clearTable();

    dto::PlanItem updateData;
    updateData.id = 99999;
    updateData.userId = m_testUserId;

    bool result = m_planItemRepository->update(updateData);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_change_plan)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_REQUIRE_GT(planItemId, 0);

    dto::PlanItem updateData;
    updateData.id = planItemId;
    updateData.planId = m_draftPlanId;

    bool result = m_planItemRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->planId, m_draftPlanId);
}

// ============================================================
// Тесты удаления элементов плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_plan_item_success)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_REQUIRE_GT(planItemId, 0);

    bool result = m_planItemRepository->remove(planItemId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_planItemRepository->exists(planItemId));
}

BOOST_AUTO_TEST_CASE(test_remove_plan_item_nonexistent)
{
    clearTable();

    bool result = m_planItemRepository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_plan_id)
{
    clearTable();

    // Создаём элементы для активного плана
    for (size_t i = 0; i < 3; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    // Создаём элемент для черновика
    auto draftItem = createTestPlanItem(m_draftPlanId, m_testItemIds[0], m_testUserId);
    m_planItemRepository->create(draftItem);

    int64_t deleted = m_planItemRepository->removeByPlanId(m_activePlanId);
    BOOST_CHECK_EQUAL(deleted, 3);

    auto items = m_planItemRepository->findByPlanId(m_activePlanId);
    BOOST_CHECK(items.empty());
}

// ============================================================
// Тесты проверки существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);

    BOOST_CHECK(m_planItemRepository->exists(planItemId));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    clearTable();

    BOOST_CHECK(!m_planItemRepository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_exists_by_plan_and_item_true)
{
    clearTable();

    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    m_planItemRepository->create(planItem);

    BOOST_CHECK(m_planItemRepository->existsByPlanAndItem(m_activePlanId, m_testItemIds[0]));
}

BOOST_AUTO_TEST_CASE(test_exists_by_plan_and_item_false)
{
    clearTable();

    BOOST_CHECK(!m_planItemRepository->existsByPlanAndItem(m_activePlanId, 99999));
}

// ============================================================
// Тесты копирования элементов между планами
// ============================================================

BOOST_AUTO_TEST_CASE(test_copy_from_plan_success)
{
    clearTable();

    // Создаём элементы для исходного плана
    for (size_t i = 0; i < 3; ++i)
    {
        auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[i], m_testUserId);
        m_planItemRepository->create(planItem);
    }

    int64_t copied = m_planItemRepository->copyFromPlan(m_activePlanId, m_draftPlanId);
    BOOST_CHECK_EQUAL(copied, 3);

    // Проверяем, что элементы скопировались в целевой план
    auto items = m_planItemRepository->findByPlanId(m_draftPlanId);
    BOOST_CHECK_EQUAL(items.size(), 3);

    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(*item.planId, m_draftPlanId);
    }
}

BOOST_AUTO_TEST_CASE(test_copy_from_plan_empty)
{
    clearTable();

    // Нет элементов в исходном плане
    int64_t copied = m_planItemRepository->copyFromPlan(m_activePlanId, m_draftPlanId);
    BOOST_CHECK_EQUAL(copied, 0);
}

BOOST_AUTO_TEST_CASE(test_copy_from_plan_invalid_source)
{
    clearTable();

    int64_t copied = m_planItemRepository->copyFromPlan(99999, m_draftPlanId);
    BOOST_CHECK_EQUAL(copied, 0);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл элемента плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_plan_item_lifecycle)
{
    clearTable();

    // 1. Создание
    auto planItem = createTestPlanItem(m_activePlanId, m_testItemIds[0], m_testUserId);
    int64_t planItemId = m_planItemRepository->create(planItem);
    BOOST_CHECK_GT(planItemId, 0);

    // 2. Чтение
    auto found = m_planItemRepository->findById(planItemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemIds[0]);

    // 3. Обновление
    dto::PlanItem updateData;
    updateData.id = planItemId;
    updateData.userId = m_secondUserId;

    BOOST_CHECK(m_planItemRepository->update(updateData));

    found = m_planItemRepository->findById(planItemId);
    BOOST_CHECK(found->userId.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_secondUserId);

    // 4. Проверка в списке
    auto [items, total] = m_planItemRepository->findAll(1, 20, m_activePlanId);
    BOOST_CHECK_GE(total, 1);

    // 5. Проверка существования
    BOOST_CHECK(m_planItemRepository->existsByPlanAndItem(m_activePlanId, m_testItemIds[0]));

    // 6. Удаление
    BOOST_CHECK(m_planItemRepository->remove(planItemId));
    BOOST_CHECK(!m_planItemRepository->exists(planItemId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
