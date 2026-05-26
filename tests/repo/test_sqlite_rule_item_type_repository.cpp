#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/rule_item_type.h"

#include "repo/sqlite/sqlite_rule_item_type_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RuleItemTypeRepositoryFixture
{
    RuleItemTypeRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_rule_item_type_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE RuleItemType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                ruleId INTEGER NOT NULL,
                itemTypeId INTEGER NOT NULL,
                isReader INTEGER NOT NULL DEFAULT 0,
                isWriter INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (ruleId) REFERENCES Rule(id) ON DELETE CASCADE,
                FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                UNIQUE(ruleId, itemTypeId)
            )
        )");
        conn->execute("CREATE TABLE Rule (id INTEGER PRIMARY KEY)");
        conn->execute("CREATE TABLE ItemType (id INTEGER PRIMARY KEY)");
        conn->execute("INSERT INTO Rule (id) VALUES (1), (2)");
        conn->execute("INSERT INTO ItemType (id) VALUES (100), (200)");

        m_repository = std::make_unique<repositories::SqliteRuleItemTypeRepository>(m_database);
    }

    dto::RuleItemType createTestRuleItemType(int64_t ruleId = 1, int64_t itemTypeId = 100, bool isReader = false, bool isWriter = false)
    {
        dto::RuleItemType rit;
        rit.ruleId = ruleId;
        rit.itemTypeId = itemTypeId;
        rit.isReader = isReader;
        rit.isWriter = isWriter;
        return rit;
    }

    ~RuleItemTypeRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRuleItemTypeRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRuleItemTypeRepositoryTests, RuleItemTypeRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto rit = createTestRuleItemType(1, 100, true, false);
    int64_t id = m_repository->create(rit);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(1, 100));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    m_repository->create(createTestRuleItemType(1, 100));
    BOOST_CHECK_THROW(m_repository->create(createTestRuleItemType(1, 100)), std::exception);
}

BOOST_AUTO_TEST_CASE(test_find_by_id)
{
    auto rit = createTestRuleItemType(2, 200, true, true);
    int64_t id = m_repository->create(rit);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->ruleId, 2);
    BOOST_CHECK_EQUAL(*found->itemTypeId, 200);
    BOOST_CHECK(found->isReader.value_or(false));
    BOOST_CHECK(found->isWriter.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_find_by_rule_id)
{
    m_repository->create(createTestRuleItemType(1, 100));
    m_repository->create(createTestRuleItemType(1, 200));
    m_repository->create(createTestRuleItemType(2, 100));

    auto list = m_repository->findByRuleId(1);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rit : list)
        BOOST_CHECK_EQUAL(*rit.ruleId, 1);
}

BOOST_AUTO_TEST_CASE(test_find_by_item_type_id)
{
    m_repository->create(createTestRuleItemType(1, 100));
    m_repository->create(createTestRuleItemType(2, 100));
    m_repository->create(createTestRuleItemType(1, 200));

    auto list = m_repository->findByItemTypeId(100);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& rit : list)
        BOOST_CHECK_EQUAL(*rit.itemTypeId, 100);
}

BOOST_AUTO_TEST_CASE(test_update)
{
    auto rit = createTestRuleItemType(1, 100);
    int64_t id = m_repository->create(rit);
    dto::RuleItemType update;
    update.id = id;
    update.isReader = true;
    update.isWriter = true;
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK(found->isReader.value_or(false));
    BOOST_CHECK(found->isWriter.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_remove)
{
    auto rit = createTestRuleItemType(1, 100);
    int64_t id = m_repository->create(rit);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(1, 100));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_filters)
{
    m_repository->create(createTestRuleItemType(1, 100));
    m_repository->create(createTestRuleItemType(1, 200));
    m_repository->create(createTestRuleItemType(2, 100));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 3);

    auto [filterByRule, totalRule] = m_repository->findAll(1, 10, 1, std::nullopt);
    BOOST_CHECK_EQUAL(totalRule, 2);

    auto [filterByItem, totalItem] = m_repository->findAll(1, 10, std::nullopt, 100);
    BOOST_CHECK_EQUAL(totalItem, 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
