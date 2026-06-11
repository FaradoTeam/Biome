#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/item.h"
#include "common/dto/item_link.h"

#include "repo/sqlite/sqlite_item_link_repository.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct ItemLinkRepositoryFixture
{
    ItemLinkRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_item_link_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute("PRAGMA foreign_keys=ON");

        // Создаём таблицы
        conn->execute(R"(
            CREATE TABLE Workflow (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                caption TEXT NOT NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE State (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                orderNumber INTEGER DEFAULT 0,
                isArchive INTEGER NOT NULL DEFAULT 0,
                isQueue INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE ItemType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                defaultStateId INTEGER,
                caption TEXT NOT NULL,
                kind TEXT NOT NULL
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
                isArchive INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER NOT NULL,
                parentId INTEGER,
                stateId INTEGER NOT NULL,
                phaseId INTEGER,
                caption TEXT NOT NULL,
                content TEXT,
                searchCaption TEXT,
                searchContent TEXT,
                isDeleted INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
            )
        )");

        conn->execute(R"(
            CREATE TABLE LinkType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sourceItemTypeId INTEGER NOT NULL,
                destinationItemTypeId INTEGER NOT NULL,
                isBidirectional INTEGER NOT NULL DEFAULT 0,
                caption TEXT NOT NULL,
                FOREIGN KEY (sourceItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                FOREIGN KEY (destinationItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE ItemLink (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                linkTypeId INTEGER NOT NULL,
                sourceItemId INTEGER NOT NULL,
                destinationItemId INTEGER NOT NULL,
                FOREIGN KEY (linkTypeId) REFERENCES LinkType(id) ON DELETE CASCADE,
                FOREIGN KEY (sourceItemId) REFERENCES Item(id) ON DELETE CASCADE,
                FOREIGN KEY (destinationItemId) REFERENCES Item(id) ON DELETE CASCADE,
                UNIQUE(linkTypeId, sourceItemId, destinationItemId)
            )
        )");

        // Заполняем тестовыми данными
        conn->execute("INSERT INTO Workflow (caption) VALUES ('Test Workflow')");
        int64_t workflowId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO State (workflowId, caption, orderNumber) VALUES ("
            + std::to_string(workflowId) + ", 'Новая', 1)"
        );
        int64_t stateId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO ItemType (workflowId, defaultStateId, caption, kind) VALUES ("
            + std::to_string(workflowId) + ", " + std::to_string(stateId) + ", 'Задача', 'issue')"
        );
        int64_t itemTypeId = conn->lastInsertId();

        conn->execute("INSERT INTO Project (caption) VALUES ('Test Project')");
        int64_t projectId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO Phase (projectId, caption) VALUES ("
            + std::to_string(projectId) + ", 'Test Phase')"
        );
        int64_t phaseId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) VALUES ("
            + std::to_string(itemTypeId) + ", " + std::to_string(stateId) + ", "
            + std::to_string(phaseId) + ", 'Элемент 1')"
        );
        m_sourceItemId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) VALUES ("
            + std::to_string(itemTypeId) + ", " + std::to_string(stateId) + ", "
            + std::to_string(phaseId) + ", 'Элемент 2')"
        );
        m_destItemId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) VALUES ("
            + std::to_string(itemTypeId) + ", " + std::to_string(stateId) + ", "
            + std::to_string(phaseId) + ", 'Элемент 3')"
        );
        m_thirdItemId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO LinkType (sourceItemTypeId, destinationItemTypeId, caption) VALUES ("
            + std::to_string(itemTypeId) + ", " + std::to_string(itemTypeId) + ", 'связан')"
        );
        m_linkTypeId = conn->lastInsertId();

        conn->execute(
            "INSERT INTO LinkType (sourceItemTypeId, destinationItemTypeId, caption) VALUES ("
            + std::to_string(itemTypeId) + ", " + std::to_string(itemTypeId) + ", 'другой')"
        );
        m_secondLinkTypeId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteItemLinkRepository>(m_database);
    }

    void clearTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM ItemLink");
    }

    dto::ItemLink createTestLink(
        int64_t linkTypeId = -1,
        int64_t sourceId = -1,
        int64_t destId = -1
    )
    {
        dto::ItemLink link;
        link.linkTypeId = (linkTypeId > 0) ? linkTypeId : m_linkTypeId;
        link.sourceItemId = (sourceId > 0) ? sourceId : m_sourceItemId;
        link.destinationItemId = (destId > 0) ? destId : m_destItemId;
        return link;
    }

    ~ItemLinkRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteItemLinkRepository> m_repository;
    int64_t m_linkTypeId = 0;
    int64_t m_secondLinkTypeId = 0;
    int64_t m_sourceItemId = 0;
    int64_t m_destItemId = 0;
    int64_t m_thirdItemId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteItemLinkRepositoryTests, ItemLinkRepositoryFixture)

// ============================================================
// Создание
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_success)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    clearTable();
    auto link = createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId);
    m_repository->create(link);
    // При повторном создании метод вернёт 0
    int64_t id = m_repository->create(link);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_missing_required_fields_fails)
{
    clearTable();
    dto::ItemLink link;
    link.linkTypeId = m_linkTypeId;
    // sourceItemId и destinationItemId отсутствуют
    int64_t id = m_repository->create(link);
    BOOST_CHECK_EQUAL(id, 0);
}

// ============================================================
// Поиск по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_REQUIRE_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, id);
    BOOST_CHECK_EQUAL(*found->linkTypeId, m_linkTypeId);
    BOOST_CHECK_EQUAL(*found->sourceItemId, m_sourceItemId);
    BOOST_CHECK_EQUAL(*found->destinationItemId, m_destItemId);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearTable();
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Поиск по элементу
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_item_id)
{
    clearTable();
    // Создаём связи:
    // 1. source = m_sourceItemId, dest = m_destItemId
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    // 2. source = m_sourceItemId, dest = m_thirdItemId
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_thirdItemId));
    // 3. source = m_thirdItemId, dest = m_sourceItemId (m_sourceItemId как destination)
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_sourceItemId));
    // 4. source = m_thirdItemId, dest = m_destItemId (не связана с m_sourceItemId)
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_destItemId));

    auto links = m_repository->findByItemId(m_sourceItemId);
    // Должны найтись 3 связи: 2 где source = m_sourceItemId и 1 где destination = m_sourceItemId
    BOOST_CHECK_EQUAL(links.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id_empty)
{
    clearTable();
    auto links = m_repository->findByItemId(99999);
    BOOST_CHECK(links.empty());
}

// ============================================================
// Поиск по типу связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_link_type_id)
{
    clearTable();
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_thirdItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_destItemId));

    auto links = m_repository->findByLinkTypeId(m_linkTypeId);
    BOOST_CHECK_EQUAL(links.size(), 2);
}

// ============================================================
// findAll с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    clearTable();
    auto [links, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(links.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearTable();
    
    // Создаём 5 записей, используя ТОЛЬКО существующие элементы
    // Используем m_destItemId и m_thirdItemId для создания разных комбинаций
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_thirdItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_thirdItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_destItemId));
    
    auto [page1, total] = m_repository->findAll(1, 3);
    BOOST_CHECK_EQUAL(total, 5);
    BOOST_CHECK_EQUAL(page1.size(), 3);
    
    auto [page2, total2] = m_repository->findAll(2, 3);
    BOOST_CHECK_EQUAL(page2.size(), 2);
    BOOST_CHECK_EQUAL(total2, 5);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_link_type)
{
    clearTable();
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_thirdItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_destItemId));

    auto [links, total] = m_repository->findAll(1, 10, m_linkTypeId, std::nullopt, std::nullopt);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.linkTypeId, m_linkTypeId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_source_item)
{
    clearTable();
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_destItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_thirdItemId));

    auto [links, total] = m_repository->findAll(1, 10, std::nullopt, m_sourceItemId, std::nullopt);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.sourceItemId, m_sourceItemId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_destination_item)
{
    clearTable();
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_destItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_sourceItemId, m_thirdItemId));

    auto [links, total] = m_repository->findAll(1, 10, std::nullopt, std::nullopt, m_destItemId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& link : links)
    {
        BOOST_CHECK_EQUAL(*link.destinationItemId, m_destItemId);
    }
}

// ============================================================
// Обновление
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_success)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_REQUIRE_GT(id, 0);

    dto::ItemLink update;
    update.id = id;
    update.sourceItemId = m_thirdItemId;
    update.destinationItemId = m_sourceItemId;

    BOOST_CHECK(m_repository->update(update));

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->sourceItemId, m_thirdItemId);
    BOOST_CHECK_EQUAL(*found->destinationItemId, m_sourceItemId);
}

BOOST_AUTO_TEST_CASE(test_update_partial)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_REQUIRE_GT(id, 0);

    dto::ItemLink update;
    update.id = id;
    update.linkTypeId = m_secondLinkTypeId;

    BOOST_CHECK(m_repository->update(update));

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->linkTypeId, m_secondLinkTypeId);
    BOOST_CHECK_EQUAL(*found->sourceItemId, m_sourceItemId);
    BOOST_CHECK_EQUAL(*found->destinationItemId, m_destItemId);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_fails)
{
    clearTable();
    dto::ItemLink update;
    update.id = 99999;
    update.sourceItemId = 1;
    BOOST_CHECK(!m_repository->update(update));
}

// ============================================================
// Удаление
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent_fails)
{
    clearTable();
    BOOST_CHECK(!m_repository->remove(99999));
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id)
{
    clearTable();
    // Создаём связи, где m_thirdItemId участвует как source
    m_repository->create(createTestLink(m_linkTypeId, m_thirdItemId, m_sourceItemId));
    m_repository->create(createTestLink(m_secondLinkTypeId, m_thirdItemId, m_destItemId));
    // Связь, где m_thirdItemId участвует как destination
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_thirdItemId));
    // Связь, не связанная с m_thirdItemId
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));

    int64_t deleted = m_repository->removeByItemId(m_thirdItemId);
    BOOST_CHECK_EQUAL(deleted, 3);

    auto links = m_repository->findByItemId(m_thirdItemId);
    BOOST_CHECK(links.empty());
}

// ============================================================
// Проверка существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearTable();
    auto link = createTestLink();
    int64_t id = m_repository->create(link);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    clearTable();
    BOOST_CHECK(!m_repository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_exists_by_triple_true)
{
    clearTable();
    m_repository->create(createTestLink(m_linkTypeId, m_sourceItemId, m_destItemId));
    BOOST_CHECK(m_repository->existsByTriple(m_linkTypeId, m_sourceItemId, m_destItemId));
}

BOOST_AUTO_TEST_CASE(test_exists_by_triple_false)
{
    clearTable();
    BOOST_CHECK(!m_repository->existsByTriple(m_linkTypeId, 999, 888));
}

// ============================================================
// Полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_lifecycle)
{
    clearTable();

    // 1. Создание
    auto link = createTestLink(m_secondLinkTypeId, m_sourceItemId, m_thirdItemId);
    int64_t id = m_repository->create(link);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->sourceItemId, m_sourceItemId);
    BOOST_CHECK_EQUAL(*found->destinationItemId, m_thirdItemId);

    // 3. Обновление
    dto::ItemLink update;
    update.id = id;
    update.destinationItemId = m_destItemId;
    BOOST_CHECK(m_repository->update(update));

    found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->destinationItemId, m_destItemId);

    // 4. Проверка в списке
    auto [list, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_GE(total, 1);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

// ============================================================
// Каскадное удаление
// ============================================================

BOOST_AUTO_TEST_CASE(test_cascade_delete_when_item_deleted)
{
    clearTable();

    // Создаём элемент, который будет удалён
    auto conn = m_database->connection();
    conn->execute(
        "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) VALUES (1, 1, 1, 'Каскадный элемент')"
    );
    int64_t cascadeItemId = conn->lastInsertId();

    // Создаём связь
    auto link = createTestLink(m_linkTypeId, cascadeItemId, m_destItemId);
    int64_t linkId = m_repository->create(link);
    BOOST_CHECK_GT(linkId, 0);

    // Удаляем элемент
    conn->execute("DELETE FROM Item WHERE id = " + std::to_string(cascadeItemId));

    // Связь должна быть удалена каскадно
    BOOST_CHECK(!m_repository->exists(linkId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
