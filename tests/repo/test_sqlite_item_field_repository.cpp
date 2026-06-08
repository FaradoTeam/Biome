#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/item_field.h"

#include "repo/sqlite/sqlite_item_field_repository.h"
#include "repo/sqlite/sqlite_item_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct ItemFieldRepositoryFixture
{
    ItemFieldRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_item_field_repo.db";
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
            CREATE TABLE FieldType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                valueType TEXT NOT NULL,
                isBoardVisible INTEGER NOT NULL DEFAULT 0
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

        conn->execute(R"(
            CREATE TABLE ItemField (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemId INTEGER NOT NULL,
                fieldTypeId INTEGER NOT NULL,
                value TEXT,
                searchValue TEXT,
                FOREIGN KEY (itemId) REFERENCES Item(id) ON DELETE CASCADE,
                FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE,
                UNIQUE(itemId, fieldTypeId)
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

        // Создаём тестовые FieldType (несколько для разных тестов)
        auto ftStmt = conn->prepareStatement(
            "INSERT INTO FieldType (itemTypeId, caption, valueType) "
            "VALUES (:itemTypeId, 'Приоритет', 'Select')"
        );
        ftStmt->bindInt64("itemTypeId", m_testItemTypeId);
        ftStmt->execute();
        m_testFieldTypeId = conn->lastInsertId();

        // Второй FieldType для тестов с разными типами
        ftStmt->reset();
        ftStmt = conn->prepareStatement(
            "INSERT INTO FieldType (itemTypeId, caption, valueType) "
            "VALUES (:itemTypeId, 'Статус', 'Select')"
        );
        ftStmt->bindInt64("itemTypeId", m_testItemTypeId);
        ftStmt->execute();
        m_testFieldTypeId2 = conn->lastInsertId();

        // Третий FieldType
        ftStmt->reset();
        ftStmt = conn->prepareStatement(
            "INSERT INTO FieldType (itemTypeId, caption, valueType) "
            "VALUES (:itemTypeId, 'Сложность', 'Select')"
        );
        ftStmt->bindInt64("itemTypeId", m_testItemTypeId);
        ftStmt->execute();
        m_testFieldTypeId3 = conn->lastInsertId();

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

        // Создаём тестовые Item (несколько для разных тестов)
        auto itemStmt = conn->prepareStatement(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) "
            "VALUES (:itemTypeId, :stateId, :phaseId, 'Тестовый элемент 1')"
        );
        itemStmt->bindInt64("itemTypeId", m_testItemTypeId);
        itemStmt->bindInt64("stateId", m_testStateId);
        itemStmt->bindInt64("phaseId", m_testPhaseId);
        itemStmt->execute();
        m_testItemId = conn->lastInsertId();

        // Второй Item
        itemStmt->reset();
        itemStmt = conn->prepareStatement(
            "INSERT INTO Item (itemTypeId, stateId, phaseId, caption) "
            "VALUES (:itemTypeId, :stateId, :phaseId, 'Тестовый элемент 2')"
        );
        itemStmt->bindInt64("itemTypeId", m_testItemTypeId);
        itemStmt->bindInt64("stateId", m_testStateId);
        itemStmt->bindInt64("phaseId", m_testPhaseId);
        itemStmt->execute();
        m_testItemId2 = conn->lastInsertId();

        m_itemRepository = std::make_unique<repositories::SqliteItemRepository>(m_database);
        m_itemFieldRepository = std::make_unique<repositories::SqliteItemFieldRepository>(m_database);
    }

    void clearItemFieldTable()
    {
        auto conn = m_database->connection();
        conn->execute("DELETE FROM ItemField");
    }

    dto::ItemField createTestField(
        const std::string& value,
        int64_t itemId,
        int64_t fieldTypeId
    )
    {
        dto::ItemField field;
        field.itemId = itemId;
        field.fieldTypeId = fieldTypeId;
        field.value = value;
        return field;
    }

    dto::ItemField createTestFieldForItem1(const std::string& value, int64_t fieldTypeId)
    {
        return createTestField(value, m_testItemId, fieldTypeId);
    }

    dto::ItemField createTestFieldForItem2(const std::string& value, int64_t fieldTypeId)
    {
        return createTestField(value, m_testItemId2, fieldTypeId);
    }

    ~ItemFieldRepositoryFixture()
    {
        m_itemRepository.reset();
        m_itemFieldRepository.reset();

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
    std::unique_ptr<repositories::SqliteItemRepository> m_itemRepository;
    std::unique_ptr<repositories::SqliteItemFieldRepository> m_itemFieldRepository;
    int64_t m_testWorkflowId = 0;
    int64_t m_testItemTypeId = 0;
    int64_t m_testStateId = 0;
    int64_t m_testPhaseId = 0;
    int64_t m_testProjectId = 0;
    int64_t m_testFieldTypeId = 0;
    int64_t m_testFieldTypeId2 = 0;
    int64_t m_testFieldTypeId3 = 0;
    int64_t m_testItemId = 0;
    int64_t m_testItemId2 = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteItemFieldRepositoryTests, ItemFieldRepositoryFixture)

// ============================================================
// Тесты создания значений полей
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_field_success)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Средний", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    BOOST_CHECK_GT(fieldId, 0);
    BOOST_CHECK(m_itemFieldRepository->exists(fieldId));

    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemId);
    BOOST_CHECK_EQUAL(*found->fieldTypeId, m_testFieldTypeId);
    BOOST_CHECK_EQUAL(*found->value, "Средний");
}

BOOST_AUTO_TEST_CASE(test_create_item_field_duplicate_fails)
{
    clearItemFieldTable();

    // Первое создание - успешно
    m_itemFieldRepository->create(createTestFieldForItem1("Высокий", m_testFieldTypeId));

    // Попытка создать дубликат (те же itemId и fieldTypeId) должна бросить исключение
    BOOST_CHECK_THROW(
        m_itemFieldRepository->create(createTestFieldForItem1("Низкий", m_testFieldTypeId)),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_item_field_missing_required_fields)
{
    clearItemFieldTable();

    dto::ItemField field;
    field.value = "Значение без itemId";

    int64_t fieldId = m_itemFieldRepository->create(field);
    BOOST_CHECK_EQUAL(fieldId, 0);
}

BOOST_AUTO_TEST_CASE(test_create_item_field_search_value_generated)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("ВЫСОКИЙ ПРИОРИТЕТ", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);
    BOOST_REQUIRE_GT(fieldId, 0);

    auto conn = m_database->connection();
    auto stmt = conn->prepareStatement(
        "SELECT searchValue FROM ItemField WHERE id = :id"
    );
    stmt->bindInt64("id", fieldId);
    auto rs = stmt->executeQuery();

    BOOST_REQUIRE(rs->next());
    BOOST_CHECK_EQUAL(rs->valueString("searchValue"), "высокий приоритет");
}

// ============================================================
// Тесты поиска значений полей
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Критический", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);
    BOOST_REQUIRE_GT(fieldId, 0);

    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, fieldId);
    BOOST_CHECK_EQUAL(*found->value, "Критический");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    clearItemFieldTable();

    auto found = m_itemFieldRepository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_item_and_field_type_success)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Уникальное значение", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    auto found = m_itemFieldRepository->findByItemAndFieldType(m_testItemId, m_testFieldTypeId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, fieldId);
    BOOST_CHECK_EQUAL(*found->value, "Уникальное значение");
}

BOOST_AUTO_TEST_CASE(test_find_by_item_and_field_type_not_found)
{
    clearItemFieldTable();

    auto found = m_itemFieldRepository->findByItemAndFieldType(99999, m_testFieldTypeId);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id)
{
    clearItemFieldTable();

    // Создаём поля для первого Item с разными fieldTypeId
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 1", m_testFieldTypeId));
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 2", m_testFieldTypeId2));

    // Создаём поле для второго Item
    m_itemFieldRepository->create(createTestFieldForItem2("Значение 3", m_testFieldTypeId));

    auto fields = m_itemFieldRepository->findByItemId(m_testItemId);
    BOOST_CHECK_EQUAL(fields.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_by_item_id_empty)
{
    clearItemFieldTable();

    auto fields = m_itemFieldRepository->findByItemId(m_testItemId2);
    BOOST_CHECK(fields.empty());
}

// ============================================================
// Тесты findAll с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    clearItemFieldTable();

    auto [fields, total] = m_itemFieldRepository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(fields.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    clearItemFieldTable();

    // Создаём поля с разными fieldTypeId для одного Item
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 1", m_testFieldTypeId));
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 2", m_testFieldTypeId2));
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 3", m_testFieldTypeId3));

    auto [page1, total] = m_itemFieldRepository->findAll(1, 2);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(page1.size(), 2);

    auto [page2, total2] = m_itemFieldRepository->findAll(2, 2);
    BOOST_CHECK_EQUAL(page2.size(), 1);
    BOOST_CHECK_EQUAL(total2, 3);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_item_id)
{
    clearItemFieldTable();

    // Создаём поля для первого Item
    m_itemFieldRepository->create(createTestFieldForItem1("Значение для Item 1", m_testFieldTypeId));
    m_itemFieldRepository->create(createTestFieldForItem1("Другое значение для Item 1", m_testFieldTypeId2));

    // Создаём поле для второго Item
    m_itemFieldRepository->create(createTestFieldForItem2("Значение для Item 2", m_testFieldTypeId));

    auto [fields, total] = m_itemFieldRepository->findAll(1, 20, m_testItemId);
    BOOST_CHECK_EQUAL(total, 2);
    BOOST_CHECK_EQUAL(fields.size(), 2);
    for (const auto& field : fields)
    {
        BOOST_CHECK_EQUAL(*field.itemId, m_testItemId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_field_type_id)
{
    clearItemFieldTable();

    // Создаём поля с разными fieldTypeId
    m_itemFieldRepository->create(createTestFieldForItem1("Высокий", m_testFieldTypeId));
    m_itemFieldRepository->create(createTestFieldForItem1("В работе", m_testFieldTypeId2));
    m_itemFieldRepository->create(createTestFieldForItem2("Низкий", m_testFieldTypeId));

    auto [fields, total] = m_itemFieldRepository->findAll(1, 20, std::nullopt, m_testFieldTypeId);
    BOOST_CHECK_EQUAL(total, 2);
    BOOST_CHECK_EQUAL(fields.size(), 2);
    for (const auto& field : fields)
    {
        BOOST_CHECK_EQUAL(*field.fieldTypeId, m_testFieldTypeId);
    }
}

// ============================================================
// Тесты обновления значений полей
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_item_field_success)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Старое значение", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    dto::ItemField updateData;
    updateData.id = fieldId;
    updateData.value = "Новое значение";

    bool result = m_itemFieldRepository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->value, "Новое значение");
}

BOOST_AUTO_TEST_CASE(test_update_item_field_partial)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Оригинал", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    dto::ItemField updateData;
    updateData.id = fieldId;
    updateData.value = "Обновлённое значение";

    BOOST_CHECK(m_itemFieldRepository->update(updateData));

    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_CHECK_EQUAL(*found->value, "Обновлённое значение");
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemId);
    BOOST_CHECK_EQUAL(*found->fieldTypeId, m_testFieldTypeId);
}

BOOST_AUTO_TEST_CASE(test_update_item_field_change_item)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Значение", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    dto::ItemField updateData;
    updateData.id = fieldId;
    updateData.itemId = m_testItemId2;

    BOOST_CHECK(m_itemFieldRepository->update(updateData));

    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_CHECK_EQUAL(*found->itemId, m_testItemId2);
}

BOOST_AUTO_TEST_CASE(test_update_item_field_nonexistent)
{
    clearItemFieldTable();

    dto::ItemField updateData;
    updateData.id = 99999;
    updateData.value = "Несуществующее";

    bool result = m_itemFieldRepository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления значений полей
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_item_field_success)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Удаляемое значение", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    bool result = m_itemFieldRepository->remove(fieldId);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_itemFieldRepository->exists(fieldId));
}

BOOST_AUTO_TEST_CASE(test_remove_item_field_nonexistent)
{
    clearItemFieldTable();

    bool result = m_itemFieldRepository->remove(99999);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id_success)
{
    clearItemFieldTable();

    // Создаём два поля для первого элемента
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 1", m_testFieldTypeId));
    m_itemFieldRepository->create(createTestFieldForItem1("Значение 2", m_testFieldTypeId2));

    int64_t deletedCount = m_itemFieldRepository->removeByItemId(m_testItemId);
    BOOST_CHECK_EQUAL(deletedCount, 2);

    auto fields = m_itemFieldRepository->findByItemId(m_testItemId);
    BOOST_CHECK(fields.empty());
}

BOOST_AUTO_TEST_CASE(test_remove_by_item_id_nonexistent)
{
    clearItemFieldTable();

    int64_t deletedCount = m_itemFieldRepository->removeByItemId(99999);
    BOOST_CHECK_EQUAL(deletedCount, 0);
}

// ============================================================
// Тесты проверки существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    clearItemFieldTable();

    auto field = createTestFieldForItem1("Существующее", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);

    BOOST_CHECK(m_itemFieldRepository->exists(fieldId));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    clearItemFieldTable();

    BOOST_CHECK(!m_itemFieldRepository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_exists_by_item_and_field_type_true)
{
    clearItemFieldTable();

    m_itemFieldRepository->create(createTestFieldForItem1("Тестовое значение", m_testFieldTypeId));

    BOOST_CHECK(m_itemFieldRepository->existsByItemAndFieldType(m_testItemId, m_testFieldTypeId));
}

BOOST_AUTO_TEST_CASE(test_exists_by_item_and_field_type_false)
{
    clearItemFieldTable();

    BOOST_CHECK(!m_itemFieldRepository->existsByItemAndFieldType(99999, m_testFieldTypeId));
}

// ============================================================
// Интеграционный тест: полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_field_lifecycle)
{
    clearItemFieldTable();

    // 1. Создание
    auto field = createTestFieldForItem1("Начальное значение", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);
    BOOST_CHECK_GT(fieldId, 0);

    // 2. Чтение
    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->value, "Начальное значение");

    // 3. Обновление
    dto::ItemField updateData;
    updateData.id = fieldId;
    updateData.value = "Обновлённое значение";
    BOOST_CHECK(m_itemFieldRepository->update(updateData));

    found = m_itemFieldRepository->findById(fieldId);
    BOOST_CHECK_EQUAL(*found->value, "Обновлённое значение");

    // 4. Проверка в списке
    auto [fields, total] = m_itemFieldRepository->findAll(1, 20, m_testItemId);
    BOOST_CHECK_GE(total, 1);

    // 5. Удаление
    BOOST_CHECK(m_itemFieldRepository->remove(fieldId));
    BOOST_CHECK(!m_itemFieldRepository->exists(fieldId));
}

// ============================================================
// Тест каскадного удаления при удалении элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_cascade_delete_when_item_deleted)
{
    clearItemFieldTable();

    // Создаём поле
    auto field = createTestFieldForItem1("Каскадное удаление", m_testFieldTypeId);
    int64_t fieldId = m_itemFieldRepository->create(field);
    BOOST_CHECK_GT(fieldId, 0);

    // Удаляем элемент (Item)
    BOOST_CHECK(m_itemRepository->hardDelete(m_testItemId));

    // Поле должно быть удалено каскадно
    auto found = m_itemFieldRepository->findById(fieldId);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
