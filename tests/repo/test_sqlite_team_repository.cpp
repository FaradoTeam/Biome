#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/team.h"

#include "repo/sqlite/sqlite_team_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct TeamRepositoryFixture
{
    TeamRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_team_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE Team (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT
            )
        )");

        m_repository = std::make_unique<repositories::SqliteTeamRepository>(m_database);
    }

    dto::Team createTestTeam(const std::string& caption = "Тестовая команда", const std::string& description = "Описание команды")
    {
        dto::Team team;
        team.caption = caption;
        team.description = description;
        return team;
    }

    ~TeamRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteTeamRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteTeamRepositoryTests, TeamRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_team_success)
{
    auto team = createTestTeam();
    int64_t id = m_repository->create(team);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_team_missing_caption_fails)
{
    dto::Team team;
    team.description = "Без названия";
    int64_t id = m_repository->create(team);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto team = createTestTeam("Найди меня", "Найди описание");
    int64_t id = m_repository->create(team);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Найди меня");
    BOOST_CHECK_EQUAL(*found->description, "Найди описание");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_team_success)
{
    auto team = createTestTeam("Старое", "Старое описание");
    int64_t id = m_repository->create(team);
    dto::Team update;
    update.id = id;
    update.caption = "Новое";
    update.description = "Новое описание";
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->caption, "Новое");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");
}

BOOST_AUTO_TEST_CASE(test_update_team_partial)
{
    auto team = createTestTeam("Оригинал", "Описание");
    int64_t id = m_repository->create(team);
    dto::Team update;
    update.id = id;
    update.caption = "Только название";
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->caption, "Только название");
    BOOST_CHECK_EQUAL(*found->description, "Описание"); // не изменилось
}

BOOST_AUTO_TEST_CASE(test_update_team_nonexistent)
{
    dto::Team update;
    update.id = 99999;
    update.caption = "Несуществующая";
    BOOST_CHECK(!m_repository->update(update));
}

BOOST_AUTO_TEST_CASE(test_remove_team_success)
{
    auto team = createTestTeam("Удали меня");
    int64_t id = m_repository->create(team);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_remove_team_nonexistent)
{
    BOOST_CHECK(!m_repository->remove(99999));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination)
{
    for (int i = 1; i <= 10; ++i)
        m_repository->create(createTestTeam("Команда " + std::to_string(i)));

    auto [page1, total] = m_repository->findAll(1, 5);
    BOOST_CHECK_EQUAL(total, 10);
    BOOST_CHECK_EQUAL(page1.size(), 5);

    auto [page2, total2] = m_repository->findAll(2, 5);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_NE(page1[0].id.value(), page2[0].id.value());
}

BOOST_AUTO_TEST_CASE(test_find_all_search)
{
    m_repository->create(createTestTeam("Альфа команда"));
    m_repository->create(createTestTeam("Бета отдел"));
    m_repository->create(createTestTeam("Гамма группа"));

    auto [result, total] = m_repository->findAll(1, 10, "команда");
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*result[0].caption, "Альфа команда");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
