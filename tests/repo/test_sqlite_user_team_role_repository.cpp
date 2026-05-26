#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/user_team_role.h"

#include "repo/sqlite/sqlite_user_team_role_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserTeamRoleRepositoryFixture
{
    UserTeamRoleRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_user_team_role_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE UserTeamRole (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                teamId INTEGER NOT NULL,
                roleId INTEGER NOT NULL,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
                FOREIGN KEY (teamId) REFERENCES Team(id) ON DELETE CASCADE,
                FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE,
                UNIQUE(userId, teamId, roleId)
            )
        )");
        // Вспомогательные таблицы для внешних ключей
        conn->execute("CREATE TABLE User (id INTEGER PRIMARY KEY)");
        conn->execute("CREATE TABLE Team (id INTEGER PRIMARY KEY)");
        conn->execute("CREATE TABLE Role (id INTEGER PRIMARY KEY)");
        conn->execute("INSERT INTO User (id) VALUES (100), (200)");
        conn->execute("INSERT INTO Team (id) VALUES (10), (20)");
        conn->execute("INSERT INTO Role (id) VALUES (1), (2)");

        m_repository = std::make_unique<repositories::SqliteUserTeamRoleRepository>(m_database);
    }

    dto::UserTeamRole createTestUserTeamRole(int64_t userId = 100, int64_t teamId = 10, int64_t roleId = 1)
    {
        dto::UserTeamRole utr;
        utr.userId = userId;
        utr.teamId = teamId;
        utr.roleId = roleId;
        return utr;
    }

    ~UserTeamRoleRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteUserTeamRoleRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserTeamRoleRepositoryTests, UserTeamRoleRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto utr = createTestUserTeamRole(100, 10, 1);
    int64_t id = m_repository->create(utr);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(100, 10));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    BOOST_CHECK_THROW(m_repository->create(createTestUserTeamRole(100, 10, 1)), std::exception);
}

BOOST_AUTO_TEST_CASE(test_find_by_id)
{
    auto utr = createTestUserTeamRole(200, 20, 2);
    int64_t id = m_repository->create(utr);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, 200);
    BOOST_CHECK_EQUAL(*found->teamId, 20);
    BOOST_CHECK_EQUAL(*found->roleId, 2);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    m_repository->create(createTestUserTeamRole(100, 20, 2));
    m_repository->create(createTestUserTeamRole(200, 10, 1));

    auto list = m_repository->findByUserId(100);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& utr : list)
        BOOST_CHECK_EQUAL(*utr.userId, 100);
}

BOOST_AUTO_TEST_CASE(test_find_by_team_id)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    m_repository->create(createTestUserTeamRole(200, 10, 2));
    m_repository->create(createTestUserTeamRole(100, 20, 1));

    auto list = m_repository->findByTeamId(10);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& utr : list)
        BOOST_CHECK_EQUAL(*utr.teamId, 10);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_and_team)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    auto found = m_repository->findByUserAndTeam(100, 10);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->roleId, 1);
    auto notFound = m_repository->findByUserAndTeam(100, 20);
    BOOST_CHECK(!notFound.has_value());
}

BOOST_AUTO_TEST_CASE(test_update)
{
    auto utr = createTestUserTeamRole(100, 10, 1);
    int64_t id = m_repository->create(utr);
    dto::UserTeamRole update;
    update.id = id;
    update.roleId = 2;
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->roleId, 2);
}

BOOST_AUTO_TEST_CASE(test_remove)
{
    auto utr = createTestUserTeamRole(100, 10, 1);
    int64_t id = m_repository->create(utr);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(100, 10));
}

BOOST_AUTO_TEST_CASE(test_remove_by_user_and_team)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    BOOST_CHECK(m_repository->removeByUserAndTeam(100, 10));
    BOOST_CHECK(!m_repository->exists(100, 10));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_filters)
{
    m_repository->create(createTestUserTeamRole(100, 10, 1));
    m_repository->create(createTestUserTeamRole(100, 20, 2));
    m_repository->create(createTestUserTeamRole(200, 10, 1));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 3);

    auto [filterUser, totalUser] = m_repository->findAll(1, 10, 100, std::nullopt, std::nullopt);
    BOOST_CHECK_EQUAL(totalUser, 2);

    auto [filterTeam, totalTeam] = m_repository->findAll(1, 10, std::nullopt, 10, std::nullopt);
    BOOST_CHECK_EQUAL(totalTeam, 2);

    auto [filterRole, totalRole] = m_repository->findAll(1, 10, std::nullopt, std::nullopt, 1);
    BOOST_CHECK_EQUAL(totalRole, 2);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
