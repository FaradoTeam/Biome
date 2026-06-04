#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/field_type_possible_value.h"

#include "repo/sqlite/sqlite_field_type_possible_value_repository.h"
#include "repo/sqlite/sqlite_field_type_repository.h"

#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct FieldTypePossibleValueRepositoryFixture
{
    FieldTypePossibleValueRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_field_type_possible_value_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему
        conn->execute(R"(
            CREATE TABLE ItemType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                defaultStateId INTEGER,
                caption TEXT NOT NULL,
                kind TEXT NOT NULL,
                defaultContent TEXT
            )
        )");

        conn->execute(R"(
            CREATE TABLE FieldType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                itemTypeId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                description TEXT,
                valueType TEXT NOT NULL,
                isBoardVisible INTEGER NOT NULL DEFAULT 0,
                FOREIGN KEY (itemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE
            )
        )");

        conn->execute(R"(
            CREATE TABLE FieldTypePossibleValue (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                fieldTypeId INTEGER NOT NULL,
                value TEXT NOT NULL,
                FOREIGN KEY (fieldTypeId) REFERENCES FieldType(id) ON DELETE CASCADE,
                UNIQUE(fieldTypeId, value)
            )
        )");

        // Создаем тестовый ItemType
        conn->execute(R"(
            INSERT INTO ItemType (workflowId, caption, kind)
            VALUES (1, 'Задача', 'issue')
        )");
        m_testItemTypeId = conn->lastInsertId();

        // Создаем тестовый FieldType
        auto stmt = conn->prepareStatement(
            "INSERT INTO FieldType (itemTypeId, caption, valueType) "
            "VALUES (:itemTypeId, :caption, :valueType)"
        );
        stmt->bindInt64("itemTypeId", m_testItemTypeId);
        stmt->bindString("caption", "Приоритет");
        stmt->bindString("valueType", "Select");
        stmt->execute();
        m_testFieldTypeId = conn->lastInsertId();

        m_repository = std::make_unique<repositories::SqliteFieldTypePossibleValueRepository>(m_database);
    }

    dto::FieldTypePossibleValue createTestValue(
        const std::string& value = "Высокий"
    )
    {
        dto::FieldTypePossibleValue val;
        val.fieldTypeId = m_testFieldTypeId;
        val.value = value;
        return val;
    }

    ~FieldTypePossibleValueRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteFieldTypePossibleValueRepository> m_repository;
    int64_t m_testItemTypeId = 0;
    int64_t m_testFieldTypeId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteFieldTypePossibleValueRepositoryTests, FieldTypePossibleValueRepositoryFixture)

// ============================================================
// Тесты создания
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto value = createTestValue("Средний");
    int64_t id = m_repository->create(value);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_duplicate_fails)
{
    m_repository->create(createTestValue("Низкий"));
    BOOST_CHECK_THROW(
        m_repository->create(createTestValue("Низкий")),
        std::exception
    );
}

BOOST_AUTO_TEST_CASE(test_create_missing_field_type_id_fails)
{
    dto::FieldTypePossibleValue value;
    value.value = "Значение";
    int64_t id = m_repository->create(value);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_empty_value_fails)
{
    auto value = createTestValue("");
    int64_t id = m_repository->create(value);
    BOOST_CHECK_EQUAL(id, 0);
}

// ============================================================
// Тесты поиска
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto value = createTestValue("Очень высокий");
    int64_t id = m_repository->create(value);
    BOOST_REQUIRE_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, id);
    BOOST_CHECK_EQUAL(*found->fieldTypeId, m_testFieldTypeId);
    BOOST_CHECK_EQUAL(*found->value, "Очень высокий");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_field_type_id)
{
    m_repository->create(createTestValue("Значение 1"));
    m_repository->create(createTestValue("Значение 2"));
    m_repository->create(createTestValue("Значение 3"));

    auto values = m_repository->findByFieldTypeId(m_testFieldTypeId);
    BOOST_CHECK_EQUAL(values.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_find_by_field_type_id_empty)
{
    auto values = m_repository->findByFieldTypeId(99999);
    BOOST_CHECK(values.empty());
}

// ============================================================
// Тесты findAll с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    auto [values, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(values.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    for (int i = 1; i <= 10; ++i)
    {
        m_repository->create(createTestValue("Значение " + std::to_string(i)));
    }

    auto [page1, total] = m_repository->findAll(1, 5);
    BOOST_CHECK_EQUAL(total, 10);
    BOOST_CHECK_EQUAL(page1.size(), 5);

    auto [page2, total2] = m_repository->findAll(2, 5);
    BOOST_CHECK_EQUAL(page2.size(), 5);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_field_type_id)
{
    // Создаем второй FieldType
    auto conn = m_database->connection();
    auto stmt = conn->prepareStatement(
        "INSERT INTO FieldType (itemTypeId, caption, valueType) "
        "VALUES (:itemTypeId, :caption, :valueType)"
    );
    stmt->bindInt64("itemTypeId", m_testItemTypeId);
    stmt->bindString("caption", "Статус");
    stmt->bindString("valueType", "Select");
    stmt->execute();
    int64_t secondFieldTypeId = conn->lastInsertId();

    m_repository->create(createTestValue("Первый"));
    m_repository->create(createTestValue("Второй"));

    dto::FieldTypePossibleValue otherValue;
    otherValue.fieldTypeId = secondFieldTypeId;
    otherValue.value = "Другое значение";
    m_repository->create(otherValue);

    auto [values, total] = m_repository->findAll(1, 20, m_testFieldTypeId);
    BOOST_CHECK_EQUAL(total, 2);
}

// ============================================================
// Тесты обновления
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_success)
{
    auto value = createTestValue("Старое значение");
    int64_t id = m_repository->create(value);
    BOOST_REQUIRE_GT(id, 0);

    dto::FieldTypePossibleValue updateData;
    updateData.id = id;
    updateData.value = "Новое значение";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(result);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->value, "Новое значение");
}

BOOST_AUTO_TEST_CASE(test_update_partial)
{
    auto value = createTestValue("Оригинальное значение");
    int64_t id = m_repository->create(value);

    dto::FieldTypePossibleValue updateData;
    updateData.id = id;
    updateData.value = "Обновленное значение";

    BOOST_CHECK(m_repository->update(updateData));

    auto found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->value, "Обновленное значение");
    BOOST_CHECK_EQUAL(*found->fieldTypeId, m_testFieldTypeId);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_fails)
{
    dto::FieldTypePossibleValue updateData;
    updateData.id = 99999;
    updateData.value = "Несуществующее";

    bool result = m_repository->update(updateData);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты удаления
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    auto value = createTestValue("Удаляемое значение");
    int64_t id = m_repository->create(value);

    bool result = m_repository->remove(id);
    BOOST_CHECK(result);
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent_fails)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты проверки существования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    auto value = createTestValue("Существующее значение");
    int64_t id = m_repository->create(value);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    BOOST_CHECK(!m_repository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_exists_by_value_true)
{
    m_repository->create(createTestValue("Уникальное значение"));
    BOOST_CHECK(m_repository->existsByValue(m_testFieldTypeId, "Уникальное значение"));
}

BOOST_AUTO_TEST_CASE(test_exists_by_value_false)
{
    BOOST_CHECK(!m_repository->existsByValue(m_testFieldTypeId, "Несуществующее значение"));
}

// ============================================================
// Интеграционный тест
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_lifecycle)
{
    // 1. Создание
    auto value = createTestValue("Жизненный цикл");
    int64_t id = m_repository->create(value);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->value, "Жизненный цикл");

    // 3. Обновление
    dto::FieldTypePossibleValue updateData;
    updateData.id = id;
    updateData.value = "Обновленный жизненный цикл";
    BOOST_CHECK(m_repository->update(updateData));

    found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->value, "Обновленный жизненный цикл");

    // 4. Проверка в списке
    auto [values, total] = m_repository->findAll(1, 20, m_testFieldTypeId);
    BOOST_CHECK_GE(total, 1);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
