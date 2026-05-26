#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/role_menu_item.h"

#include "repo/sqlite/sqlite_role_menu_item_repository.h"

#include "storage/database_factory.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct RoleMenuItemRepositoryFixture
{
    RoleMenuItemRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_role_menu_item_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute(R"(
            CREATE TABLE RoleMenuItem (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                roleId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                link TEXT,
                icon TEXT,
                FOREIGN KEY (roleId) REFERENCES Role(id) ON DELETE CASCADE
            )
        )");
        conn->execute("CREATE TABLE Role (id INTEGER PRIMARY KEY)");
        conn->execute("INSERT INTO Role (id) VALUES (1), (2)");

        m_repository = std::make_unique<repositories::SqliteRoleMenuItemRepository>(m_database);
    }

    dto::RoleMenuItem createTestMenuItem(int64_t roleId = 1, const std::string& caption = "Пункт", const std::string& link = "/link", const std::string& icon = "icon.png")
    {
        dto::RoleMenuItem item;
        item.roleId = roleId;
        item.caption = caption;
        item.link = link;
        item.icon = icon;
        return item;
    }

    ~RoleMenuItemRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteRoleMenuItemRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteRoleMenuItemRepositoryTests, RoleMenuItemRepositoryFixture)

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto item = createTestMenuItem(1, "Проекты", "/projects", "folder");
    int64_t id = m_repository->create(item);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_missing_required_fields)
{
    dto::RoleMenuItem item;
    item.roleId = 1;
    // caption и link отсутствуют
    BOOST_CHECK_EQUAL(m_repository->create(item), 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_id)
{
    auto item = createTestMenuItem(2, "Настройки", "/settings", "gear");
    int64_t id = m_repository->create(item);
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->roleId, 2);
    BOOST_CHECK_EQUAL(*found->caption, "Настройки");
    BOOST_CHECK_EQUAL(*found->link, "/settings");
    BOOST_CHECK_EQUAL(*found->icon, "gear");
}

BOOST_AUTO_TEST_CASE(test_find_by_role_id)
{
    m_repository->create(createTestMenuItem(1, "Пункт1", "/a"));
    m_repository->create(createTestMenuItem(1, "Пункт2", "/b"));
    m_repository->create(createTestMenuItem(2, "Пункт3", "/c"));

    auto list = m_repository->findByRoleId(1);
    BOOST_CHECK_EQUAL(list.size(), 2);
    for (auto& item : list)
        BOOST_CHECK_EQUAL(*item.roleId, 1);
}

BOOST_AUTO_TEST_CASE(test_update)
{
    auto item = createTestMenuItem(1, "Старый", "/old", "old.png");
    int64_t id = m_repository->create(item);
    dto::RoleMenuItem update;
    update.id = id;
    update.caption = "Новый";
    update.link = "/new";
    BOOST_CHECK(m_repository->update(update));
    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->caption, "Новый");
    BOOST_CHECK_EQUAL(*found->link, "/new");
    // icon остался прежним
    BOOST_CHECK_EQUAL(*found->icon, "old.png");
}

BOOST_AUTO_TEST_CASE(test_remove)
{
    auto item = createTestMenuItem(1, "Удалить", "/del");
    int64_t id = m_repository->create(item);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination_and_filter)
{
    for (int i = 1; i <= 5; ++i)
        m_repository->create(createTestMenuItem(1, "Пункт " + std::to_string(i), "/item" + std::to_string(i)));

    auto [all, totalAll] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(totalAll, 5);

    auto [filtered, totalFiltered] = m_repository->findAll(1, 10, 1);
    BOOST_CHECK_EQUAL(totalFiltered, 5);

    auto [page, totalPage] = m_repository->findAll(1, 3);
    BOOST_CHECK_EQUAL(page.size(), 3);
    BOOST_CHECK_EQUAL(totalPage, 5);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
