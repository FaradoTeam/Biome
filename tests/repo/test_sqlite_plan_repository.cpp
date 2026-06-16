#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/plan.h"

#include "repo/sqlite/sqlite_phase_repository.h"
#include "repo/sqlite/sqlite_plan_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct PlanRepositoryFixture
{
    PlanRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_plan_repo.db";
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

        // Создаём тестового пользователя
        conn->execute(
            "INSERT INTO User (login, email, passwordHash) "
            "VALUES ('testuser', 'test@example.com', 'hash')"
        );
        m_testUserId = conn->lastInsertId();

        // Создаём тестовый проект
        conn->execute(
            "INSERT INTO Project (caption) VALUES ('Test Project')"
        );
        m_testProjectId = conn->lastInsertId();

        // Создаём тестовую фазу
        auto phaseStmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) VALUES (:projectId, 'Test Phase')"
        );
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_testPhaseId = conn->lastInsertId();

        // Создаём вторую фазу для тестов
        phaseStmt->reset();
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_secondPhaseId = conn->lastInsertId();

        m_planRepository = std::make_unique<repositories::SqlitePlanRepository>(m_database);
    }

    void clearTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM Plan");
    }

    dto::Plan createTestPlan(
        int64_t phaseId,
        const std::string& caption = "Тестовый план",
        std::optional<int64_t> basePlanId = std::nullopt,
        bool isActive = false,
        std::optional<int64_t> activatedByUserId = std::nullopt
    )
    {
        dto::Plan plan;
        plan.phaseId = phaseId;
        if (basePlanId.has_value())
            plan.basePlanId = basePlanId;
        plan.caption = caption;
        plan.description = "Описание тестового плана";
        plan.isActive = isActive;
        plan.createdByUserId = m_testUserId;
        if (activatedByUserId.has_value())
            plan.activatedByUserId = activatedByUserId;
        return plan;
    }

    ~PlanRepositoryFixture()
    {
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
    std::unique_ptr<repositories::SqlitePlanRepository> m_planRepository;
    int64_t m_testUserId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_secondPhaseId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqlitePlanRepositoryTests, PlanRepositoryFixture)

// ============================================================
// Тесты создания плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_plan_success)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId);
    int64_t planId = m_planRepository->create(plan);

    BOOST_CHECK_GT(planId, 0);
    BOOST_CHECK(m_planRepository->exists(planId));

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->phaseId, m_testPhaseId);
    BOOST_CHECK(!found->basePlanId.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Тестовый план");
    BOOST_CHECK_EQUAL(*found->description, "Описание тестового плана");
    BOOST_CHECK(!found->isActive.value_or(true));
    BOOST_CHECK_EQUAL(*found->createdByUserId, m_testUserId);
    BOOST_CHECK(found->createdAt.has_value());
    BOOST_CHECK(!found->activatedAt.has_value());
    BOOST_CHECK(!found->activatedByUserId.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_plan_with_base_plan)
{
    clearTable();

    // Создаём базовый план
    auto basePlan = createTestPlan(m_testPhaseId, "Базовый план");
    int64_t basePlanId = m_planRepository->create(basePlan);
    BOOST_REQUIRE_GT(basePlanId, 0);

    // Создаём план на основе базового
    auto childPlan = createTestPlan(m_testPhaseId, "Дочерний план", basePlanId);
    int64_t childPlanId = m_planRepository->create(childPlan);

    BOOST_CHECK_GT(childPlanId, 0);
    BOOST_CHECK(m_planRepository->exists(childPlanId));

    auto found = m_planRepository->findById(childPlanId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->basePlanId.has_value());
    BOOST_CHECK_EQUAL(*found->basePlanId, basePlanId);
}

BOOST_AUTO_TEST_CASE(test_create_plan_without_caption_fails)
{
    clearTable();

    dto::Plan plan;
    plan.phaseId = m_testPhaseId;
    plan.createdByUserId = m_testUserId;

    int64_t planId = m_planRepository->create(plan);
    BOOST_CHECK_EQUAL(planId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_plan_without_phase_id_fails)
{
    clearTable();

    dto::Plan plan;
    plan.caption = "План без фазы";
    plan.createdByUserId = m_testUserId;

    int64_t planId = m_planRepository->create(plan);
    BOOST_CHECK_EQUAL(planId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_plan_without_created_by_user_id_fails)
{
    clearTable();

    dto::Plan plan;
    plan.phaseId = m_testPhaseId;
    plan.caption = "План без создателя";

    int64_t planId = m_planRepository->create(plan);
    BOOST_CHECK_EQUAL(planId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_active_plan)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "Активный план", std::nullopt, true);
    int64_t planId = m_planRepository->create(plan);

    BOOST_CHECK_GT(planId, 0);

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->isActive.value_or(false));
}

// ============================================================
// Тесты поиска плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "План для поиска");
    int64_t planId = m_planRepository->create(plan);
    BOOST_REQUIRE_GT(planId, 0);

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, planId);
    BOOST_CHECK_EQUAL(*found->caption, "План для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearTable();

    auto found = m_planRepository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_active_by_phase_id_success)
{
    clearTable();

    // Создаём неактивный план
    auto inactivePlan = createTestPlan(m_testPhaseId, "Неактивный план");
    m_planRepository->create(inactivePlan);

    // Создаём активный план
    auto activePlan = createTestPlan(m_testPhaseId, "Активный план", std::nullopt, true);
    int64_t activePlanId = m_planRepository->create(activePlan);

    auto found = m_planRepository->findActiveByPhaseId(m_testPhaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, activePlanId);
    BOOST_CHECK(found->isActive.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_find_active_by_phase_id_no_active_plan)
{
    clearTable();

    // Создаём только неактивные планы
    auto plan1 = createTestPlan(m_testPhaseId, "План 1");
    auto plan2 = createTestPlan(m_testPhaseId, "План 2");
    m_planRepository->create(plan1);
    m_planRepository->create(plan2);

    auto found = m_planRepository->findActiveByPhaseId(m_testPhaseId);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_active_by_phase_id_wrong_phase)
{
    clearTable();

    // Создаём активный план для первой фазы
    auto activePlan = createTestPlan(m_testPhaseId, "Активный план", std::nullopt, true);
    m_planRepository->create(activePlan);

    // Ищем активный план для второй фазы
    auto found = m_planRepository->findActiveByPhaseId(m_secondPhaseId);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты получения списка планов с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    clearTable();

    auto [plans, total] = m_planRepository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(plans.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearTable();

    // Создаём 15 планов
    for (int i = 1; i <= 15; ++i)
    {
        auto plan = createTestPlan(m_testPhaseId, "План " + std::to_string(i));
        m_planRepository->create(plan);
    }

    // Первая страница (10 элементов)
    auto [page1, total] = m_planRepository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    // Вторая страница (5 элементов)
    auto [page2, total2] = m_planRepository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);

    // Проверяем, что элементы на разных страницах
    BOOST_CHECK_NE(page1[0].id.value(), page2[0].id.value());
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_phase_id)
{
    clearTable();

    // Создаём планы для первой фазы
    for (int i = 1; i <= 3; ++i)
    {
        auto plan = createTestPlan(m_testPhaseId, "План фазы 1_" + std::to_string(i));
        m_planRepository->create(plan);
    }

    // Создаём планы для второй фазы
    for (int i = 1; i <= 2; ++i)
    {
        auto plan = createTestPlan(m_secondPhaseId, "План фазы 2_" + std::to_string(i));
        m_planRepository->create(plan);
    }

    auto [plans, total] = m_planRepository->findAll(1, 20, m_testPhaseId);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(plans.size(), 3);

    for (const auto& plan : plans)
    {
        BOOST_CHECK_EQUAL(*plan.phaseId, m_testPhaseId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_is_active)
{
    clearTable();

    // Создаём активные планы
    for (int i = 1; i <= 3; ++i)
    {
        auto plan = createTestPlan(m_testPhaseId, "Активный " + std::to_string(i), std::nullopt, true);
        m_planRepository->create(plan);
    }

    // Создаём неактивные планы
    for (int i = 1; i <= 2; ++i)
    {
        auto plan = createTestPlan(m_testPhaseId, "Неактивный " + std::to_string(i));
        m_planRepository->create(plan);
    }

    auto [activePlans, totalActive] = m_planRepository->findAll(1, 20, std::nullopt, true);
    BOOST_CHECK_EQUAL(totalActive, 3);
    BOOST_CHECK_EQUAL(activePlans.size(), 3);

    for (const auto& plan : activePlans)
    {
        BOOST_CHECK(plan.isActive.value_or(false));
    }

    auto [inactivePlans, totalInactive] = m_planRepository->findAll(1, 20, std::nullopt, false);
    BOOST_CHECK_EQUAL(totalInactive, 2);
    BOOST_CHECK_EQUAL(inactivePlans.size(), 2);

    for (const auto& plan : inactivePlans)
    {
        BOOST_CHECK(!plan.isActive.value_or(true));
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_phase_and_active)
{
    clearTable();

    // Активный план в первой фазе
    auto plan1 = createTestPlan(m_testPhaseId, "Активный в фазе 1", std::nullopt, true);
    m_planRepository->create(plan1);

    // Неактивный план в первой фазе
    auto plan2 = createTestPlan(m_testPhaseId, "Неактивный в фазе 1");
    m_planRepository->create(plan2);

    // Активный план во второй фазе
    auto plan3 = createTestPlan(m_secondPhaseId, "Активный в фазе 2", std::nullopt, true);
    m_planRepository->create(plan3);

    auto [plans, total] = m_planRepository->findAll(1, 20, m_testPhaseId, true);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(plans.size(), 1);
    BOOST_CHECK_EQUAL(*plans[0].caption, "Активный в фазе 1");
}

// ============================================================
// Тесты обновления плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_plan_success)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "Старое название");
    int64_t planId = m_planRepository->create(plan);
    BOOST_REQUIRE_GT(planId, 0);

    dto::Plan updateData;
    updateData.id = planId;
    updateData.caption = "Новое название";
    updateData.description = "Новое описание";

    bool result = m_planRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новое название");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");
}

BOOST_AUTO_TEST_CASE(test_update_plan_partial)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "Оригинал");
    plan.description = "Оригинальное описание";
    int64_t planId = m_planRepository->create(plan);

    dto::Plan updateData;
    updateData.id = planId;
    updateData.caption = "Обновлённое название";

    bool result = m_planRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Обновлённое название");
    BOOST_CHECK_EQUAL(*found->description, "Оригинальное описание");
}

BOOST_AUTO_TEST_CASE(test_update_plan_activate)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "План для активации");
    int64_t planId = m_planRepository->create(plan);
    BOOST_REQUIRE_GT(planId, 0);

    auto now = std::chrono::system_clock::now();

    dto::Plan updateData;
    updateData.id = planId;
    updateData.isActive = true;
    updateData.activatedAt = now;
    updateData.activatedByUserId = m_testUserId;

    bool result = m_planRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->isActive.value_or(false));
    BOOST_CHECK(found->activatedAt.has_value());
    BOOST_CHECK(found->activatedByUserId.has_value());
    BOOST_CHECK_EQUAL(*found->activatedByUserId, m_testUserId);
}

BOOST_AUTO_TEST_CASE(test_update_plan_change_base_plan)
{
    clearTable();

    auto basePlan = createTestPlan(m_testPhaseId, "Базовый план");
    int64_t basePlanId = m_planRepository->create(basePlan);

    auto childPlan = createTestPlan(m_testPhaseId, "Дочерний план");
    int64_t childPlanId = m_planRepository->create(childPlan);

    dto::Plan updateData;
    updateData.id = childPlanId;
    updateData.basePlanId = basePlanId;

    bool result = m_planRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_planRepository->findById(childPlanId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->basePlanId.has_value());
    BOOST_CHECK_EQUAL(*found->basePlanId, basePlanId);
}

BOOST_AUTO_TEST_CASE(test_update_plan_nonexistent)
{
    clearTable();

    dto::Plan updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующий план";

    bool result = m_planRepository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_plan_success)
{
    clearTable();

    auto plan = createTestPlan(m_testPhaseId, "План для удаления");
    int64_t planId = m_planRepository->create(plan);
    BOOST_REQUIRE_GT(planId, 0);

    bool result = m_planRepository->remove(planId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_planRepository->exists(planId));
}

BOOST_AUTO_TEST_CASE(test_remove_plan_nonexistent)
{
    clearTable();

    bool result = m_planRepository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_deactivate_all_by_phase_id)
{
    clearTable();

    // Создаём активные планы для первой фазы
    for (int i = 1; i <= 3; ++i)
    {
        auto plan = createTestPlan(m_testPhaseId, "Активный " + std::to_string(i), std::nullopt, true);
        m_planRepository->create(plan);
    }

    // Создаём активный план для второй фазы
    auto otherPhasePlan = createTestPlan(m_secondPhaseId, "Активный в другой фазе", std::nullopt, true);
    m_planRepository->create(otherPhasePlan);

    int64_t deactivated = m_planRepository->deactivateAllByPhaseId(m_testPhaseId);
    BOOST_CHECK_EQUAL(deactivated, 3);

    // Проверяем, что планы первой фазы деактивированы
    auto [plans, total] = m_planRepository->findAll(1, 20, m_testPhaseId);
    for (const auto& plan : plans)
    {
        BOOST_CHECK(!plan.isActive.value_or(true));
    }

    // Проверяем, что план второй фазы остался активным
    auto otherPlan = m_planRepository->findById(*otherPhasePlan.id);
    BOOST_REQUIRE(otherPlan.has_value());
    BOOST_CHECK(otherPlan->isActive.value_or(false));
}

// ============================================================
// Тесты вспомогательных методов
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearTable();

    int64_t planId = m_planRepository->create(createTestPlan(m_testPhaseId));
    BOOST_CHECK(m_planRepository->exists(planId));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    clearTable();

    BOOST_CHECK(!m_planRepository->exists(99999));
}

// ============================================================
// Интеграционный тест: полный жизненный цикл плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_plan_lifecycle)
{
    clearTable();

    // 1. Создание
    auto plan = createTestPlan(m_testPhaseId, "Жизненный цикл");
    int64_t planId = m_planRepository->create(plan);
    BOOST_CHECK_GT(planId, 0);

    // 2. Чтение
    auto found = m_planRepository->findById(planId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл");

    // 3. Обновление
    dto::Plan updateData;
    updateData.id = planId;
    updateData.caption = "Обновлённый жизненный цикл";
    BOOST_CHECK(m_planRepository->update(updateData));

    found = m_planRepository->findById(planId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновлённый жизненный цикл");

    // 4. Активация
    updateData.isActive = true;
    updateData.activatedByUserId = m_testUserId;
    BOOST_CHECK(m_planRepository->update(updateData));

    found = m_planRepository->findById(planId);
    BOOST_CHECK(found->isActive.value_or(false));

    // 5. Проверка в списке
    auto [plans, total] = m_planRepository->findAll(1, 20);
    BOOST_CHECK_GE(total, 1);

    // 6. Удаление
    BOOST_CHECK(m_planRepository->remove(planId));
    BOOST_CHECK(!m_planRepository->exists(planId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
