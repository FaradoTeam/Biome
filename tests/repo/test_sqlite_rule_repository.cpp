#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/rule.h"

#include "repo/sqlite/sqlite_rule_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RuleRepositoryFixture
{
    RuleRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_rule_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE Rule (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                roleId INTEGER NOT NULL,
                isRootProjectCreator INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE
            )
        )");
        // Для внешнего ключа нужна таблица Role
        conn->execute("CREATE TABLE Role (id INTEGER PRIMARY KEY, caption TEXT)");
        conn->execute("INSERT INTO Role (id, caption) VALUES (1, 'Роль для правила')");

        m_repository = std::make_unique<repositories::SqliteRuleRepository>(m_database);
    }

    dto::Rule createTestRule(int64_t roleId = 1, bool isRootProjectCreator = false)
    {
        dto::Rule rule;
        rule.roleId = roleId;
        rule.isRootProjectCreator = isRootProjectCreator;
        return rule;
    }

    ~RuleRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRuleRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRuleRepositoryTests, RuleRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_rule_success)
{
    auto rule = createTestRule(1, true);
    int64_t id = m_repository->create(rule);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_rule_missing_roleId_fails)
{
    dto::Rule rule;
    int64_t id = m_repository->create(rule);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto rule = createTestRule(1, true);
    int64_t id = m_repository->create(rule);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->roleId, 1);
    BOOST_CHECK(found->isRootProjectCreator.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_find_by_role_id_success)
{
    m_repository->create(createTestRule(1, false));
    auto found = m_repository->findByRoleId(1);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->isRootProjectCreator.value_or(true));
}

BOOST_AUTO_TEST_CASE(test_update_rule_success)
{
    auto rule = createTestRule(1, false);
    int64_t id = m_repository->create(rule);
    dto::Rule update;
    update.id = id;
    update.isRootProjectCreator = true;
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK(found->isRootProjectCreator.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_remove_rule_success)
{
    auto rule = createTestRule(1);
    int64_t id = m_repository->create(rule);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_and_filter)
{
    m_repository->create(createTestRule(1, true));
    m_repository->create(createTestRule(1, false));
    m_repository->create(createTestRule(1, false));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 3);

    auto [filtered, totalFiltered] = m_repository->findAll(1, 10, 1);
    BOOST_CHECK_EQUAL(totalFiltered, 3);

    auto [page, totalPage] = m_repository->findAll(1, 2);
    BOOST_CHECK_EQUAL(page.size(), 2);
    BOOST_CHECK_EQUAL(totalPage, 3);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
