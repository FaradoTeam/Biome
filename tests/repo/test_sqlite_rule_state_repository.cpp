#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/rule_state.h"

#include "repo/sqlite/sqlite_rule_state_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RuleStateRepositoryFixture
{
    RuleStateRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_rule_state_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE RuleState (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ruleId INTEGER NOT NULL,
                stateId INTEGER NOT NULL,
                isStateAllowed INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                UNIQUE(ruleId, stateId)
            )
        )");
        conn->execute("CREATE TABLE Rule (id INTEGER PRIMARY KEY)");
        conn->execute("CREATE TABLE State (id INTEGER PRIMARY KEY)");
        conn->execute("INSERT INTO Rule (id) VALUES (1), (2)");
        conn->execute("INSERT INTO State (id) VALUES (10), (20)");

        m_repository = std::make_unique<repositories::SqliteRuleStateRepository>(m_database);
    }

    dto::RuleState createTestRuleState(int64_t ruleId = 1, int64_t stateId = 10, bool allowed = false)
    {
        dto::RuleState rs;
        rs.ruleId = ruleId;
        rs.stateId = stateId;
        rs.isStateAllowed = allowed;
        return rs;
    }

    ~RuleStateRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRuleStateRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRuleStateRepositoryTests, RuleStateRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto rs = createTestRuleState(1, 10, true);
    int64_t id = m_repository->create(rs);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    m_repository->create(createTestRuleState(1, 10));
    BOOST_CHECK_THROW(m_repository->create(createTestRuleState(1, 10)), std::exception);
}

BOOST_AUTO_TEST_CASE(test_find_by_id)
{
    auto rs = createTestRuleState(2, 20, true);
    int64_t id = m_repository->create(rs);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->ruleId, 2);
    BOOST_CHECK_EQUAL(*found->stateId, 20);
    BOOST_CHECK(found->isStateAllowed.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_find_by_rule_id)
{
    m_repository->create(createTestRuleState(1, 10));
    m_repository->create(createTestRuleState(1, 20));
    m_repository->create(createTestRuleState(2, 10));

    auto list = m_repository->findByRuleId(1);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rs : list)
        BOOST_CHECK_EQUAL(*rs.ruleId, 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_state_id)
{
    m_repository->create(createTestRuleState(1, 10));
    m_repository->create(createTestRuleState(2, 10));
    m_repository->create(createTestRuleState(1, 20));

    auto list = m_repository->findByStateId(10);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rs : list)
        BOOST_CHECK_EQUAL(*rs.stateId, 10);
}

BOOST_AUTO_TEST_CASE(test_update)
{
    auto rs = createTestRuleState(1, 10);
    int64_t id = m_repository->create(rs);
    dto::RuleState update;
    update.id = id;
    update.isStateAllowed = true;
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK(found->isStateAllowed.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_remove)
{
    auto rs = createTestRuleState(1, 10);
    int64_t id = m_repository->create(rs);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(1, 10));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_filters)
{
    m_repository->create(createTestRuleState(1, 10));
    m_repository->create(createTestRuleState(1, 20));
    m_repository->create(createTestRuleState(2, 10));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 3);

    auto [filterRule, totalRule] = m_repository->findAll(1, 10, 1, std::nullopt);
    BOOST_CHECK_EQUAL(totalRule, 2);

    auto [filterState, totalState] = m_repository->findAll(1, 10, std::nullopt, 10);
    BOOST_CHECK_EQUAL(totalState, 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
