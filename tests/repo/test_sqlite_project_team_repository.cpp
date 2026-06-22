#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/project_team.h"

#include "repo/sqlite/sqlite_project_team_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct ProjectTeamRepositoryFixture
{
    ProjectTeamRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_project_team_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE Project (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0
            )
        )");
        conn->execute(R"(
            CREATE TABLE Team (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT
            )
        )");
        conn->execute(R"(
            CREATE TABLE ProjectTeam (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                projectId INTEGER NOT NULL,
                teamId INTEGER NOT NULL,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
                FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,
                UNIQUE(projectId, teamId)
            )
        )");

        conn->execute("INSERT INTO Project (id, caption) VALUES (1, 'Project 1')");
        conn->execute("INSERT INTO Project (id, caption) VALUES (2, 'Project 2')");
        conn->execute("INSERT INTO Team (id, caption) VALUES (10, 'Team 10')");
        conn->execute("INSERT INTO Team (id, caption) VALUES (20, 'Team 20')");
        conn->execute("INSERT INTO Team (id, caption) VALUES (30, 'Team 30')");

        m_repository = std::make_unique<repositories::SqliteProjectTeamRepository>(m_database);
    }

    void clearTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM ProjectTeam");
    }

    dto::ProjectTeam createTestProjectTeam(
        int64_t projectId = 1,
        int64_t teamId = 10
    )
    {
        dto::ProjectTeam pt;
        pt.projectId = projectId;
        pt.teamId = teamId;
        return pt;
    }

    ~ProjectTeamRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteProjectTeamRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteProjectTeamRepositoryTests, ProjectTeamRepositoryFixture)

// ============================================================
// Тесты создания
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_success)
{
    clearTable();
    auto pt = createTestProjectTeam(1, 10);
    int64_t id = m_repository->create(pt);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    // При нарушении UNIQUE constraint метод create() бросает исключение
    BOOST_CHECK_THROW(
        m_repository->create(createTestProjectTeam(1, 10)),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_missing_project_id_fails)
{
    clearTable();
    dto::ProjectTeam pt;
    pt.teamId = 10;
    // Репозиторий проверяет наличие полей и возвращает 0
    int64_t id = m_repository->create(pt);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_missing_team_id_fails)
{
    clearTable();
    dto::ProjectTeam pt;
    pt.projectId = 1;
    int64_t id = m_repository->create(pt);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_non_existent_project_fails)
{
    clearTable();
    auto pt = createTestProjectTeam(999, 10);
    // При нарушении FOREIGN KEY constraint метод create() бросает исключение
    BOOST_CHECK_THROW(
        m_repository->create(pt),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_non_existent_team_fails)
{
    clearTable();
    auto pt = createTestProjectTeam(1, 999);
    // При нарушении FOREIGN KEY constraint метод create() бросает исключение
    BOOST_CHECK_THROW(
        m_repository->create(pt),
        std::exception
    );
}

// ============================================================
// Тесты поиска
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearTable();
    auto pt = createTestProjectTeam(2, 20);
    int64_t id = m_repository->create(pt);
    BOOST_REQUIRE_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, id);
    BOOST_CHECK_EQUAL(*found->projectId, 2);
    BOOST_CHECK_EQUAL(*found->teamId, 20);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearTable();
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    clearTable();
    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(items.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    m_repository->create(createTestProjectTeam(1, 20));
    m_repository->create(createTestProjectTeam(2, 10));
    m_repository->create(createTestProjectTeam(2, 20));

    auto [page1, total] = m_repository->findAll(1, 2);
    BOOST_CHECK_EQUAL(total, 4);
    BOOST_CHECK_EQUAL(page1.size(), 2);

    auto [page2, total2] = m_repository->findAll(2, 2);
    BOOST_CHECK_EQUAL(total2, 4);
    BOOST_CHECK_EQUAL(page2.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_project_id)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    m_repository->create(createTestProjectTeam(1, 20));
    m_repository->create(createTestProjectTeam(2, 10));

    auto [items, total] = m_repository->findAll(1, 20, 1);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(*item.projectId, 1);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_team_id)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    m_repository->create(createTestProjectTeam(2, 10));
    m_repository->create(createTestProjectTeam(1, 20));

    auto [items, total] = m_repository->findAll(1, 20, std::nullopt, 10);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(*item.teamId, 10);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_both_project_and_team)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    m_repository->create(createTestProjectTeam(1, 20));
    m_repository->create(createTestProjectTeam(2, 10));

    auto [items, total] = m_repository->findAll(1, 20, 1, 10);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*items[0].projectId, 1);
    BOOST_CHECK_EQUAL(*items[0].teamId, 10);
}

// ============================================================
// Тесты проверки существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    BOOST_CHECK(m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_exists_false_by_project)
{
    clearTable();
    m_repository->create(createTestProjectTeam(2, 10));
    BOOST_CHECK(!m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_exists_false_by_team)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 20));
    BOOST_CHECK(!m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_exists_false_both)
{
    clearTable();
    BOOST_CHECK(!m_repository->exists(1, 10));
}

// ============================================================
// Тесты удаления
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    clearTable();
    auto pt = createTestProjectTeam(1, 10);
    int64_t id = m_repository->create(pt);
    BOOST_REQUIRE_GT(id, 0);

    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent_fails)
{
    clearTable();
    BOOST_CHECK(!m_repository->remove(99999));
}

BOOST_AUTO_TEST_CASE(test_remove_by_id_and_check_all)
{
    clearTable();
    m_repository->create(createTestProjectTeam(1, 10));
    m_repository->create(createTestProjectTeam(1, 20));
    m_repository->create(createTestProjectTeam(2, 10));

    // Удаляем связь 1-10
    auto found = m_repository->findById(1);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(m_repository->remove(*found->id));

    // Проверяем, что удалилась только одна запись
    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 2);

    // Проверяем, что остались правильные записи
    bool found1_20 = false;
    bool found2_10 = false;
    for (const auto& item : items)
    {
        if (*item.projectId == 1 && *item.teamId == 20)
            found1_20 = true;
        if (*item.projectId == 2 && *item.teamId == 10)
            found2_10 = true;
    }
    BOOST_CHECK(found1_20);
    BOOST_CHECK(found2_10);
}

// ============================================================
// Тесты каскадного удаления
// ============================================================

BOOST_AUTO_TEST_CASE(test_cascade_delete_when_project_deleted)
{
    clearTable();
    // Создаём временный проект
    auto conn = m_database->connection();
    conn->execute("INSERT INTO Project (id, caption) VALUES (100, 'Temp Project')");

    m_repository->create(createTestProjectTeam(100, 10));
    m_repository->create(createTestProjectTeam(100, 20));

    // Удаляем проект
    conn->execute("DELETE FROM Project WHERE id = 100");

    // Проверяем, что связи удалились каскадно
    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
}

BOOST_AUTO_TEST_CASE(test_cascade_delete_when_team_deleted)
{
    clearTable();
    // Создаём временную команду
    auto conn = m_database->connection();
    conn->execute("INSERT INTO Team (id, caption) VALUES (100, 'Temp Team')");

    m_repository->create(createTestProjectTeam(1, 100));
    m_repository->create(createTestProjectTeam(2, 100));

    // Удаляем команду
    conn->execute("DELETE FROM Team WHERE id = 100");

    // Проверяем, что связи удалились каскадно
    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
}

// ============================================================
// Полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_lifecycle)
{
    clearTable();

    // 1. Создание
    auto pt = createTestProjectTeam(2, 30);
    int64_t id = m_repository->create(pt);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->projectId, 2);
    BOOST_CHECK_EQUAL(*found->teamId, 30);

    // 3. Проверка существования
    BOOST_CHECK(m_repository->exists(2, 30));

    // 4. Проверка в списке
    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*items[0].id, id);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(2, 30));
    BOOST_CHECK(!m_repository->findById(id).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
