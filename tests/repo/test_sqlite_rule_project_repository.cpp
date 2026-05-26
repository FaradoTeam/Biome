#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/rule_project.h"

#include "repo/sqlite/sqlite_rule_project_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RuleProjectRepositoryFixture
{
    RuleProjectRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_rule_project_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE RuleProject (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ruleId INTEGER NOT NULL,
                projectId INTEGER NOT NULL,
                isReader INTEGER NOT NULL DEFAULT 0,
                isWriter INTEGER NOT NULL DEFAULT 0,
                isProjectEditor INTEGER NOT NULL DEFAULT 0,
                isPhaseEditor INTEGER NOT NULL DEFAULT 0,
                isBoardEditor INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE,
                UNIQUE(ruleId, projectId)
            )
        )");
        // Вспомогательные таблицы для внешних ключей
        conn->execute("CREATE TABLE Rule (id INTEGER PRIMARY KEY)");
        conn->execute("CREATE TABLE Project (id INTEGER PRIMARY KEY)");
        conn->execute("INSERT INTO Rule (id) VALUES (1), (2)");
        conn->execute("INSERT INTO Project (id) VALUES (10), (20)");

        m_repository = std::make_unique<repositories::SqliteRuleProjectRepository>(m_database);
    }

    dto::RuleProject createTestRuleProject(int64_t ruleId = 1, int64_t projectId = 10, bool isReader = false, bool isWriter = false, bool isProjectEditor = false, bool isPhaseEditor = false, bool isBoardEditor = false)
    {
        dto::RuleProject rp;
        rp.ruleId = ruleId;
        rp.projectId = projectId;
        rp.isReader = isReader;
        rp.isWriter = isWriter;
        rp.isProjectEditor = isProjectEditor;
        rp.isPhaseEditor = isPhaseEditor;
        rp.isBoardEditor = isBoardEditor;
        return rp;
    }

    ~RuleProjectRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRuleProjectRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRuleProjectRepositoryTests, RuleProjectRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_rule_project_success)
{
    auto rp = createTestRuleProject(1, 10, true, false, true, false, false);
    int64_t id = m_repository->create(rp);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_create_rule_project_duplicate_fails)
{
    m_repository->create(createTestRuleProject(1, 10));
    BOOST_CHECK_THROW(m_repository->create(createTestRuleProject(1, 10)), std::exception);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto rp = createTestRuleProject(1, 20, true, true, false, false, true);
    int64_t id = m_repository->create(rp);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->ruleId, 1);
    BOOST_CHECK_EQUAL(*found->projectId, 20);
    BOOST_CHECK(found->isReader.value_or(false));
    BOOST_CHECK(found->isWriter.value_or(false));
    BOOST_CHECK(!found->isProjectEditor.value_or(true));
    BOOST_CHECK(found->isBoardEditor.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_find_by_rule_id)
{
    m_repository->create(createTestRuleProject(1, 10));
    m_repository->create(createTestRuleProject(1, 20));
    m_repository->create(createTestRuleProject(2, 10));

    auto list = m_repository->findByRuleId(1);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rp : list)
        BOOST_CHECK_EQUAL(*rp.ruleId, 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_project_id)
{
    m_repository->create(createTestRuleProject(1, 10));
    m_repository->create(createTestRuleProject(2, 10));
    m_repository->create(createTestRuleProject(1, 20));

    auto list = m_repository->findByProjectId(10);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rp : list)
        BOOST_CHECK_EQUAL(*rp.projectId, 10);
}

BOOST_AUTO_TEST_CASE(test_update_rule_project)
{
    auto rp = createTestRuleProject(1, 10);
    int64_t id = m_repository->create(rp);
    dto::RuleProject update;
    update.id = id;
    update.isReader = true;
    update.isBoardEditor = true;
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK(found->isReader.value_or(false));
    BOOST_CHECK(found->isBoardEditor.value_or(false));
    BOOST_CHECK(!found->isWriter.value_or(true)); // не менялось
}

BOOST_AUTO_TEST_CASE(test_remove_rule_project)
{
    auto rp = createTestRuleProject(1, 10);
    int64_t id = m_repository->create(rp);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_and_filters)
{
    m_repository->create(createTestRuleProject(1, 10));
    m_repository->create(createTestRuleProject(1, 20));
    m_repository->create(createTestRuleProject(2, 10));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 3);

    auto [filteredByRule, totalRule] = m_repository->findAll(1, 10, 1, std::nullopt);
    BOOST_CHECK_EQUAL(totalRule, 2);

    auto [filteredByProject, totalProj] = m_repository->findAll(1, 10, std::nullopt, 10);
    BOOST_CHECK_EQUAL(totalProj, 2);

    auto [page, totalPage] = m_repository->findAll(1, 2);
    BOOST_CHECK_EQUAL(page.size(), 2);
    BOOST_CHECK_EQUAL(totalPage, 3);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
