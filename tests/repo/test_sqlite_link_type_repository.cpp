#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/link_type.h"

#include "repo/sqlite/sqlite_link_type_repository.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct LinkTypeRepositoryFixture
{
    LinkTypeRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_link_type_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();
        conn->execute("PRAGMA foreign_keys=ON");

        // Создаём таблицу ItemType
        conn->execute(R"(
            CREATE TABLE ItemType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                workflowId INTEGER NOT NULL,
                defaultStateId INTEGER,
                caption TEXT NOT NULL,
                kind TEXT NOT NULL
            )
        )");

        // Создаём тестовые ItemType
        conn->execute("INSERT INTO ItemType (workflowId, caption, kind) VALUES (1, 'Задача', 'issue')");
        m_sourceItemTypeId = conn->lastInsertId();

        conn->execute("INSERT INTO ItemType (workflowId, caption, kind) VALUES (1, 'Требование', 'requirement')");
        m_destItemTypeId = conn->lastInsertId();

        conn->execute("INSERT INTO ItemType (workflowId, caption, kind) VALUES (1, 'Тест-кейс', 'test-case')");
        m_otherItemTypeId = conn->lastInsertId();

        // Создаём таблицу LinkType
        conn->execute(R"(
            CREATE TABLE LinkType (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                sourceItemTypeId INTEGER NOT NULL,
                destinationItemTypeId INTEGER NOT NULL,
                isBidirectional INTEGER NOT NULL DEFAULT 0,
                caption TEXT NOT NULL,
                FOREIGN KEY (sourceItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                FOREIGN KEY (destinationItemTypeId) REFERENCES ItemType(id) ON DELETE CASCADE,
                UNIQUE(sourceItemTypeId, destinationItemTypeId, caption)
            )
        )");

        // Создаём таблицу ItemLink для проверки isUsed
        conn->execute(R"(
            CREATE TABLE ItemLink (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                linkTypeId INTEGER NOT NULL,
                sourceItemId INTEGER NOT NULL,
                destinationItemId INTEGER NOT NULL,
                FOREIGN KEY (linkTypeId) REFERENCES LinkType(id) ON DELETE CASCADE
            )
        )");

        m_repository = std::make_unique<repositories::SqliteLinkTypeRepository>(m_database);

        // Создаём тестовые данные для всех тестов
        // Для проверки фильтрации создаём несколько записей
        dto::LinkType lt1;
        lt1.sourceItemTypeId = m_sourceItemTypeId;
        lt1.destinationItemTypeId = m_destItemTypeId;
        lt1.caption = "связан";
        lt1.isBidirectional = false;
        m_repository->create(lt1);

        dto::LinkType lt2;
        lt2.sourceItemTypeId = m_sourceItemTypeId;
        lt2.destinationItemTypeId = m_otherItemTypeId;
        lt2.caption = "ссылается";
        lt2.isBidirectional = true;
        m_repository->create(lt2);

        dto::LinkType lt3;
        lt3.sourceItemTypeId = m_otherItemTypeId;
        lt3.destinationItemTypeId = m_destItemTypeId;
        lt3.caption = "блокирует";
        lt3.isBidirectional = false;
        m_repository->create(lt3);
    }

    dto::LinkType createTestLinkType(
        const std::string& caption = "тестовый",
        bool isBidirectional = false,
        std::optional<int64_t> source = std::nullopt,
        std::optional<int64_t> dest = std::nullopt
    )
    {
        dto::LinkType linkType;
        linkType.caption = caption;
        linkType.isBidirectional = isBidirectional;
        linkType.sourceItemTypeId = source.has_value() ? *source : m_sourceItemTypeId;
        linkType.destinationItemTypeId = dest.has_value() ? *dest : m_destItemTypeId;
        return linkType;
    }

    ~LinkTypeRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteLinkTypeRepository> m_repository;
    int64_t m_sourceItemTypeId = 0;
    int64_t m_destItemTypeId = 0;
    int64_t m_otherItemTypeId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqliteLinkTypeRepositoryTests, LinkTypeRepositoryFixture)

// ============================================================
// Создание
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_success)
{
    auto lt = createTestLinkType("новый тип");
    int64_t id = m_repository->create(lt);
    BOOST_CHECK_GT(id, 0);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_create_missing_required_fields_fails)
{
    dto::LinkType lt;
    lt.caption = "Без типов";
    int64_t id = m_repository->create(lt);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_empty_caption_fails)
{
    auto lt = createTestLinkType("");
    int64_t id = m_repository->create(lt);
    BOOST_CHECK_EQUAL(id, 0);
}

// ============================================================
// Поиск
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto lt = createTestLinkType("найти меня");
    int64_t id = m_repository->create(lt);
    BOOST_REQUIRE_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, id);
    BOOST_CHECK_EQUAL(*found->caption, "найти меня");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// findAll с пагинацией и фильтрацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // В фикстуре уже есть 3 записи, добавим ещё 2
    for (int i = 1; i <= 2; ++i)
    {
        m_repository->create(createTestLinkType("Дополнительный " + std::to_string(i)));
    }

    auto [page1, total] = m_repository->findAll(1, 3);
    BOOST_CHECK_EQUAL(total, 5);
    BOOST_CHECK_EQUAL(page1.size(), 3);

    auto [page2, total2] = m_repository->findAll(2, 3);
    BOOST_CHECK_EQUAL(page2.size(), 2);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_source_type)
{
    auto [list, total] = m_repository->findAll(1, 10, m_sourceItemTypeId, std::nullopt);
    // В фикстуре: lt1 и lt2 имеют source = m_sourceItemTypeId
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& lt : list)
    {
        BOOST_CHECK_EQUAL(*lt.sourceItemTypeId, m_sourceItemTypeId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_dest_type)
{
    auto [list, total] = m_repository->findAll(1, 10, std::nullopt, m_destItemTypeId);
    // В фикстуре: lt1 и lt3 имеют dest = m_destItemTypeId
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& lt : list)
    {
        BOOST_CHECK_EQUAL(*lt.destinationItemTypeId, m_destItemTypeId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_both_types)
{
    auto [list, total] = m_repository->findAll(1, 10, m_sourceItemTypeId, m_destItemTypeId);
    // Только lt1 подходит
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(*list[0].caption, "связан");
}

// ============================================================
// Обновление
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_success)
{
    auto lt = createTestLinkType("Старое название");
    int64_t id = m_repository->create(lt);
    BOOST_REQUIRE_GT(id, 0);

    dto::LinkType update;
    update.id = id;
    update.caption = "Новое название";
    update.isBidirectional = true;

    BOOST_CHECK(m_repository->update(update));

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Новое название");
    BOOST_CHECK(found->isBidirectional.value_or(false));
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_fails)
{
    dto::LinkType update;
    update.id = 99999;
    update.caption = "Несуществующий";
    BOOST_CHECK(!m_repository->update(update));
}

// ============================================================
// Удаление
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_success)
{
    auto lt = createTestLinkType("Удаляемый тип");
    int64_t id = m_repository->create(lt);
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_remove_in_use_fails)
{
    // Создаём тип связи
    auto lt = createTestLinkType("Используемый тип");
    int64_t linkTypeId = m_repository->create(lt);
    BOOST_REQUIRE_GT(linkTypeId, 0);

    // Создаём ItemLink, использующий этот тип
    auto conn = m_database->connection();
    conn->execute(
        "INSERT INTO ItemLink (linkTypeId, sourceItemId, destinationItemId) "
        "VALUES ("
        + std::to_string(linkTypeId) + ", 1, 2)"
    );

    BOOST_CHECK(!m_repository->remove(linkTypeId));
    BOOST_CHECK(m_repository->exists(linkTypeId));
}

BOOST_AUTO_TEST_CASE(test_remove_nonexistent_fails)
{
    BOOST_CHECK(!m_repository->remove(99999));
}

// ============================================================
// Проверка существования и использования
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    auto lt = createTestLinkType("Существующий");
    int64_t id = m_repository->create(lt);
    BOOST_CHECK(m_repository->exists(id));
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    BOOST_CHECK(!m_repository->exists(99999));
}

BOOST_AUTO_TEST_CASE(test_is_used_true)
{
    auto lt = createTestLinkType("Используемый");
    int64_t linkTypeId = m_repository->create(lt);
    auto conn = m_database->connection();
    conn->execute(
        "INSERT INTO ItemLink (linkTypeId, sourceItemId, destinationItemId) "
        "VALUES ("
        + std::to_string(linkTypeId) + ", 10, 20)"
    );
    BOOST_CHECK(m_repository->isUsed(linkTypeId));
}

BOOST_AUTO_TEST_CASE(test_is_used_false)
{
    auto lt = createTestLinkType("Неиспользуемый");
    int64_t id = m_repository->create(lt);
    BOOST_CHECK(!m_repository->isUsed(id));
}

// ============================================================
// Полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_lifecycle)
{
    // 1. Создание
    dto::LinkType lt = createTestLinkType("Жизненный цикл", true);
    int64_t id = m_repository->create(lt);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->caption, "Жизненный цикл");

    // 3. Обновление
    dto::LinkType update;
    update.id = id;
    update.caption = "Обновлённый цикл";
    BOOST_CHECK(m_repository->update(update));
    found = m_repository->findById(id);
    BOOST_CHECK_EQUAL(*found->caption, "Обновлённый цикл");

    // 4. Проверка в списке
    auto [list, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_GE(total, 1);

    // 5. Удаление
    BOOST_CHECK(m_repository->remove(id));
    BOOST_CHECK(!m_repository->exists(id));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
