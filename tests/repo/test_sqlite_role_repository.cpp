#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/role.h"

#include "repo/sqlite/sqlite_role_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RoleRepositoryFixture
{
    RoleRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_role_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE Role (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL,
                description TEXT
            )
        )");

        m_repository = std::make_unique<repositories::SqliteRoleRepository>(m_database);
    }

    dto::Role createTestRole(const std::string& caption = "Тестовая роль", const std::string& description = "Описание роли")
    {
        dto::Role role;
        role.caption = caption;
        role.description = description;
        return role;
    }

    ~RoleRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRoleRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRoleRepositoryTests, RoleRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_role_success)
{
    auto role = createTestRole();
    int64_t id = m_repository->create(role);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_role_missing_caption_fails)
{
    dto::Role role;
    role.description = "Без названия";
    int64_t id = m_repository->create(role);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto role = createTestRole("Роль1", "Описание1");
    int64_t id = m_repository->create(role);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Роль1");
    BOOST_CHECK_EQUAL(*found->description, "Описание1");
}

BOOST_AUTO_TEST_CASE(test_find_by_caption_success)
{
    m_repository->create(createTestRole("Уникальная роль"));
    auto found = m_repository->findByCaption("Уникальная роль");
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Уникальная роль");
}

BOOST_AUTO_TEST_CASE(test_find_by_caption_not_found)
{
    auto found = m_repository->findByCaption("Не существует");
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_role_success)
{
    auto role = createTestRole("Старая роль");
    int64_t id = m_repository->create(role);
    dto::Role update;
    update.id = id;
    update.caption = "Новая роль";
    update.description = "Новое описание";
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->caption, "Новая роль");
    BOOST_CHECK_EQUAL(*found->description, "Новое описание");
}

BOOST_AUTO_TEST_CASE(test_remove_role_success)
{
    auto role = createTestRole("Удаляемая роль");
    int64_t id = m_repository->create(role);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_and_search)
{
    for (int i = 1; i <= 5; ++i)
        m_repository->create(createTestRole("Роль " + std::to_string(i)));

    auto [page, total] = m_repository->findAll(1, 3);
    BOOST_CHECK_EQUAL(total, 5);
    BOOST_CHECK_EQUAL(page.size(), 3);

    auto [searchResult, searchTotal] = m_repository->findAll(1, 10, "Роль 2");
    BOOST_CHECK_EQUAL(searchTotal, 1);
    BOOST_CHECK_EQUAL(*searchResult[0].caption, "Роль 2");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
