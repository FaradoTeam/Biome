#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/item.h"
#include "common/helpers/time_helpers.h"

#include "repo/sqlite/sqlite_item_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct ItemRepositoryFixture
{
    ItemRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_item_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Включаем поддержку внешних ключей
        conn->execute("PRAGMA foreign_keys=ON");

        // Создаём схему для тестов
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
            CREATE TABLE Phase (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                projectId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0,
                searchCaption TEXT,
                searchDescription TEXT
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
                FOREIGN KEY (parentId) REFERENCES Item(id) ON DELETE SET NULL,
                FOREIGN KEY (stateId) REFERENCES State(id) ON DELETE CASCADE,
                FOREIGN KEY (phaseId) REFERENCES Phase(id) ON DELETE SET NULL
            )
        )");

        // Создаём тестовый Workflow
        conn->execute("INSERT INTO Workflow (caption) VALUES ('Test Workflow')");
        m_testWorkflowId = conn->lastInsertId();

        // Создаём тестовое State
        auto stmt = conn->prepareStatement(
            "INSERT INTO State (workflowId, caption, orderNumber) "
            "VALUES (:workflowId, 'Новая', 1)"
        );
        stmt->bindInt64("workflowId", m_testWorkflowId);
        stmt->execute();
        m_testStateId = conn->lastInsertId();

        // Создаём тестовый ItemType
        auto itStmt = conn->prepareStatement(
            "INSERT INTO ItemType (workflowId, defaultStateId, caption, kind) "
            "VALUES (:workflowId, :defaultStateId, 'Задача', 'issue')"
        );
        itStmt->bindInt64("workflowId", m_testWorkflowId);
        itStmt->bindInt64("defaultStateId", m_testStateId);
        itStmt->execute();
        m_testItemTypeId = conn->lastInsertId();

        // Создаём тестовый Project
        conn->execute("INSERT INTO Project (caption) VALUES ('Test Project')");
        m_testProjectId = conn->lastInsertId();

        // Создаём тестовую Phase
        auto phaseStmt = conn->prepareStatement(
            "INSERT INTO Phase (projectId, caption) VALUES (:projectId, 'Test Phase')"
        );
        phaseStmt->bindInt64("projectId", m_testProjectId);
        phaseStmt->execute();
        m_testPhaseId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteItemRepository>(m_database);
    }

    dto::Item createTestItem(
        const std::string& caption = "Тестовый элемент",
        std::optional<int64_t> parentId = std::nullopt,
        std::optional<bool> isDeleted = std::nullopt
    )
    {
        dto::Item item;
        item.itemTypeId = m_testItemTypeId;
        item.stateId = m_testStateId;
        item.phaseId = m_testPhaseId;
        item.caption = caption;
        item.content = "Содержимое тестового элемента";
        if (parentId.has_value())
            item.parentId = parentId;
        if (isDeleted.has_value())
            item.isDeleted = isDeleted;
        return item;
    }

    ~ItemRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteItemRepository> m_repository;
    int64_t m_testWorkflowId = 0;
    int64_t m_testItemTypeId = 0;
    int64_t m_testStateId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_testProjectId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteItemRepositoryTests, ItemRepositoryFixture)

// ============================================================
// Тесты создания элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_success)
{
    auto item = createTestItem();
    int64_t itemId = m_repository->create(item);

    BOOST_CHECK_GT(itemId, 0);
    BOOST_CHECK(m_repository->exists(itemId));

    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Тестовый элемент");
    BOOST_CHECK_EQUAL(*found->itemTypeId, m_testItemTypeId);
    BOOST_CHECK_EQUAL(*found->stateId, m_testStateId);
    BOOST_CHECK_EQUAL(*found->phaseId, m_testPhaseId);
    BOOST_CHECK(!found->isDeleted.value_or(true));
}

BOOST_AUTO_TEST_CASE(test_create_item_missing_required_fields)
{
    dto::Item item;
    item.caption = "Без типа элемента";

    int64_t itemId = m_repository->create(item);
    BOOST_CHECK_EQUAL(itemId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_item_with_parent)
{
    auto parentItem = createTestItem("Родитель");
    int64_t parentId = m_repository->create(parentItem);

    auto childItem = createTestItem("Ребёнок", parentId);
    int64_t childId = m_repository->create(childItem);

    BOOST_CHECK_GT(childId, 0);
    auto found = m_repository->findById(childId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->parentId.has_value());
    BOOST_CHECK_EQUAL(*found->parentId, parentId);
}

BOOST_AUTO_TEST_CASE(test_create_item_search_fields_generated)
{
    auto item = createTestItem("ТЕСТОВЫЙ ЭЛЕМЕНТ");
    item.content = "СОДЕРЖИМОЕ ЭЛЕМЕНТА";

    int64_t itemId = m_repository->create(item);
    BOOST_REQUIRE_GT(itemId, 0);

    auto conn = m_database->connection();
    auto stmt = conn->prepareStatement(
        "SELECT searchCaption, searchContent FROM Item WHERE id = :id"
    );
    stmt->bindInt64("id", itemId);
    auto rs = stmt->executeQuery();

    BOOST_REQUIRE(rs->next());
    BOOST_CHECK_EQUAL(rs->valueString("searchCaption"), "тестовый элемент");
    BOOST_CHECK_EQUAL(rs->valueString("searchContent"), "содержимое элемента");
}

// ============================================================
// Тесты поиска элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto item = createTestItem("Элемент для поиска");
    int64_t itemId = m_repository->create(item);
    BOOST_REQUIRE_GT(itemId, 0);

    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, itemId);
    BOOST_CHECK_EQUAL(*found->caption, "Элемент для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты пагинации и фильтрации
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    auto [items, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(items.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    for (int i = 1; i <= 15; ++i)
    {
        m_repository->create(createTestItem("Элемент " + std::to_string(i)));
    }

    auto [page1, total] = m_repository->findAll(1, 10);
    BOOST_CHECK_EQUAL(total, 15);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 5);
    BOOST_CHECK_EQUAL(total2, 15);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_item_type)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    m_repository->create(createTestItem("Задача 1"));
    m_repository->create(createTestItem("Задача 2"));

    auto [items, total] = m_repository->findAll(1, 20, m_testItemTypeId);
    BOOST_CHECK_EQUAL(total, 2);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_parent)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    auto parent = createTestItem("Родитель");
    int64_t parentId = m_repository->create(parent);

    m_repository->create(createTestItem("Ребёнок 1", parentId));
    m_repository->create(createTestItem("Ребёнок 2", parentId));
    m_repository->create(createTestItem("Другой элемент"));

    auto [children, total] = m_repository->findAll(1, 20, std::nullopt, parentId);
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& child : children)
    {
        BOOST_CHECK(child.parentId.has_value());
        BOOST_CHECK_EQUAL(*child.parentId, parentId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_phase)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    // Создаём элемент с тестовой фазой
    m_repository->create(createTestItem("Элемент в фазе"));

    // Создаём элемент с другой фазой
    auto item = createTestItem("Элемент в другой фазе");
    // Создаём другую фазу
    auto phaseStmt = conn->prepareStatement(
        "INSERT INTO Phase (projectId, caption) VALUES (:projectId, 'Another Phase')"
    );
    phaseStmt->bindInt64("projectId", m_testProjectId);
    phaseStmt->execute();
    int64_t anotherPhaseId = conn->lastInsertId();

    item.phaseId = anotherPhaseId;
    m_repository->create(item);

    // Ищем элементы в тестовой фазе
    auto [items, total] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, m_testPhaseId);
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(items.size(), 1);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_state)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    m_repository->create(createTestItem("Элемент 1"));

    auto [items, total] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, std::nullopt, m_testStateId);
    BOOST_CHECK_EQUAL(total, 1);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_deleted)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    auto activeItem = createTestItem("Активный");
    activeItem.isDeleted = false;
    m_repository->create(activeItem);

    auto deletedItem = createTestItem("Удалённый");
    deletedItem.isDeleted = true;
    m_repository->create(deletedItem);

    auto [active, totalActive] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, std::nullopt, std::nullopt, false);
    BOOST_CHECK_EQUAL(totalActive, 1);

    auto [deleted, totalDeleted] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, std::nullopt, std::nullopt, true);
    BOOST_CHECK_EQUAL(totalDeleted, 1);
}

BOOST_AUTO_TEST_CASE(test_find_all_search_by_caption)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    m_repository->create(createTestItem("Альфа задача"));
    m_repository->create(createTestItem("Бета проект"));
    m_repository->create(createTestItem("Гамма задание"));

    auto [items, total] = m_repository->findAll(1, 20, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "задача");
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*items[0].caption, "Альфа задача");
}

// ============================================================
// Тесты обновления элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_item_success)
{
    auto item = createTestItem("Старое название");
    int64_t itemId = m_repository->create(item);

    dto::Item updateData;
    updateData.id = itemId;
    updateData.caption = "Новое название";
    updateData.content = "Новое содержимое";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новое название");
    BOOST_CHECK_EQUAL(*found->content, "Новое содержимое");
}

BOOST_AUTO_TEST_CASE(test_update_item_partial)
{
    auto item = createTestItem("Оригинал");
    item.content = "Оригинальное содержимое";
    int64_t itemId = m_repository->create(item);

    dto::Item updateData;
    updateData.id = itemId;
    updateData.caption = "Обновлённое название";

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(itemId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновлённое название");
    BOOST_CHECK_EQUAL(*found->content, "Оригинальное содержимое");
}

BOOST_AUTO_TEST_CASE(test_update_item_move_to_different_parent)
{
    auto parent1 = createTestItem("Родитель 1");
    int64_t parent1Id = m_repository->create(parent1);

    auto parent2 = createTestItem("Родитель 2");
    int64_t parent2Id = m_repository->create(parent2);

    auto child = createTestItem("Ребёнок", parent1Id);
    int64_t childId = m_repository->create(child);

    dto::Item updateData;
    updateData.id = childId;
    updateData.parentId = parent2Id;

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(childId);
    BOOST_CHECK(found->parentId.has_value());
    BOOST_CHECK_EQUAL(*found->parentId, parent2Id);
}

BOOST_AUTO_TEST_CASE(test_update_item_nonexistent)
{
    dto::Item updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующий";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_soft_delete_success)
{
    auto item = createTestItem("Элемент для удаления");
    int64_t itemId = m_repository->create(item);

    BOOST_CHECK(m_repository->softDelete(itemId));

    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->isDeleted.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_restore_success)
{
    auto item = createTestItem("Элемент для восстановления");
    int64_t itemId = m_repository->create(item);
    m_repository->softDelete(itemId);

    BOOST_CHECK(m_repository->restore(itemId));

    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->isDeleted.value_or(true));
}

BOOST_AUTO_TEST_CASE(test_hard_delete_success)
{
    auto item = createTestItem("Элемент для полного удаления");
    int64_t itemId = m_repository->create(item);

    BOOST_CHECK(m_repository->hardDelete(itemId));
    BOOST_CHECK(!m_repository->exists(itemId));
}

BOOST_AUTO_TEST_CASE(test_soft_delete_nonexistent)
{
    bool result = m_repository->softDelete(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты вспомогательных методов
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_children)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    auto parent = createTestItem("Родитель");
    int64_t parentId = m_repository->create(parent);

    m_repository->create(createTestItem("Ребёнок 1", parentId));
    m_repository->create(createTestItem("Ребёнок 2", parentId));
    m_repository->create(createTestItem("Другой элемент"));

    auto children = m_repository->findChildren(parentId);
    BOOST_CHECK_EQUAL(children.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_children_include_deleted)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    auto parent = createTestItem("Родитель");
    int64_t parentId = m_repository->create(parent);

    auto activeChild = createTestItem("Активный ребёнок", parentId);
    int64_t activeId = m_repository->create(activeChild);

    auto deletedChild = createTestItem("Удалённый ребёнок", parentId);
    int64_t deletedId = m_repository->create(deletedChild);
    m_repository->softDelete(deletedId);

    auto activeOnly = m_repository->findChildren(parentId, false);
    BOOST_CHECK_EQUAL(activeOnly.size(), 1);
    BOOST_CHECK_EQUAL(*activeOnly[0].id, activeId);

    auto withDeleted = m_repository->findChildren(parentId, true);
    BOOST_CHECK_EQUAL(withDeleted.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_root_items)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    // Создаём корневые элементы (без parentId)
    m_repository->create(createTestItem("Корневой 1"));
    m_repository->create(createTestItem("Корневой 2"));

    // Создаём родительский элемент (тоже корневой, так как parentId не установлен)
    auto parent = createTestItem("Родительский элемент");
    int64_t parentId = m_repository->create(parent);

    // Создаём дочерний элемент (с parentId)
    m_repository->create(createTestItem("Дочерний элемент", parentId));

    // Получаем корневые элементы (без parentId)
    auto rootItems = m_repository->findRootItems(m_testPhaseId);

    // Должно быть 3 корневых элемента: "Корневой 1", "Корневой 2", "Родительский элемент"
    BOOST_CHECK_EQUAL(rootItems.size(), 3);

    // Проверяем, что все найденные элементы не имеют parentId
    for (const auto& item : rootItems)
    {
        BOOST_CHECK(!item.parentId.has_value());
    }
}

// ============================================================
// Интеграционный тест: полный жизненный цикл элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_lifecycle)
{
    // Очищаем таблицу перед тестом
    auto conn = m_database->connection();
    conn->execute("DELETE FROM Item");

    // 1. Создание
    auto item = createTestItem("Жизненный цикл");
    int64_t itemId = m_repository->create(item);
    BOOST_CHECK_GT(itemId, 0);

    // 2. Чтение
    auto found = m_repository->findById(itemId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл");

    // 3. Обновление
    dto::Item updateData;
    updateData.id = itemId;
    updateData.caption = "Обновлённый жизненный цикл";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(itemId);
    BOOST_CHECK_EQUAL(*found->caption, "Обновлённый жизненный цикл");

    // 4. Мягкое удаление
    BOOST_CHECK(m_repository->softDelete(itemId));
    found = m_repository->findById(itemId);
    BOOST_CHECK(found->isDeleted.value_or(false));

    // 5. Восстановление
    BOOST_CHECK(m_repository->restore(itemId));
    found = m_repository->findById(itemId);
    BOOST_CHECK(!found->isDeleted.value_or(true));

    // 6. Полное удаление
    BOOST_CHECK(m_repository->hardDelete(itemId));
    BOOST_CHECK(!m_repository->exists(itemId));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
