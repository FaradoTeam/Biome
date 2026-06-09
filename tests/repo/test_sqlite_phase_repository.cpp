#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/phase.h"

#include "repo/sqlite/sqlite_phase_repository.h"

#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct PhaseRepositoryFixture
{
    PhaseRepositoryFixture()
    {
        // Создаем временный файл для тестовой БД
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_phase_repo.db";

        // Удаляем файл если он существует от предыдущего запуска
        std::remove(m_tempDbPath.c_str());

        // Создаем файловую БД
        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        // Используем ОДНО соединение для создания схемы
        auto conn = m_database->connection();

        // Создаем схему для проектов и фаз
        conn->execute(R"(
            CREATE TABLE Project (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                parentId INTEGER,
                caption TEXT NOT NULL,
                description TEXT,
                searchCaption TEXT,
                searchDescription TEXT,
                createdAt INTEGER NOT NULL,
                updatedAt INTEGER NOT NULL,
                isArchive INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE Phase (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                projectId INTEGER NOT NULL,
                caption TEXT NOT NULL,
                description TEXT,
                beginDate INTEGER,
                endDate INTEGER,
                isArchive INTEGER NOT NULL DEFAULT 0,
                searchCaption TEXT,
                searchDescription TEXT,
                FOREIGN KEY (projectId) REFERENCES Project(id) ON DELETE CASCADE
            )
        )");

        // Создаем тестовый проект для фаз
        conn->execute(R"(
            INSERT INTO Project (caption, createdAt, updatedAt)
            VALUES ('Тестовый проект', strftime('%s', 'now'), strftime('%s', 'now'))
        )");
        m_testProjectId = conn->lastInsertId();

        // Создаем репозиторий
        m_repository = std::make_unique<repositories::SqlitePhaseRepository>(m_database);
    }

    /**
     * @brief Создает тестовую фазу.
     */
    dto::Phase createTestPhase(
        const std::string& caption = "Тестовая фаза",
        std::optional<int64_t> projectId = std::nullopt
    )
    {
        dto::Phase phase;
        phase.caption = caption;
        phase.description = "Описание тестовой фазы";
        phase.projectId = projectId.has_value() ? projectId.value() : m_testProjectId;
        phase.isArchive = false;

        // Устанавливаем даты
        auto now = std::chrono::system_clock::now();
        phase.beginDate = now;
        phase.endDate = now + std::chrono::hours(24 * 30); // +30 дней

        return phase;
    }

    ~PhaseRepositoryFixture()
    {
        // Важно: сначала уничтожаем репозиторий, потом БД
        m_repository.reset();

        if (m_database)
        {
            m_database->shutdown();
            m_database.reset();
        }

        // Удаляем временный файл БД
        if (!m_tempDbPath.empty() && std::filesystem::exists(m_tempDbPath))
        {
            std::error_code ec;
            std::filesystem::remove(m_tempDbPath, ec);
        }
    }

    std::filesystem::path m_tempDbPath;
    std::shared_ptr<db::SqliteDatabase> m_database;
    std::unique_ptr<repositories::SqlitePhaseRepository> m_repository;
    int64_t m_testProjectId = 0;
};

BOOST_FIXTURE_TEST_SUITE(SqlitePhaseRepositoryTests, PhaseRepositoryFixture)

// ============================================================
// Тесты создания фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_phase_success)
{
    // Arrange
    auto phase = createTestPhase();

    // Act
    int64_t phaseId = m_repository->create(phase);

    // Assert
    BOOST_CHECK_GT(phaseId, 0);
    BOOST_CHECK(m_repository->exists(phaseId));

    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->caption.has_value());
    BOOST_CHECK_EQUAL(found->caption.value(), "Тестовая фаза");
    BOOST_CHECK(found->description.has_value());
    BOOST_CHECK_EQUAL(found->description.value(), "Описание тестовой фазы");
    BOOST_CHECK(found->projectId.has_value());
    BOOST_CHECK_EQUAL(found->projectId.value(), m_testProjectId);
    BOOST_CHECK(found->beginDate.has_value());
    BOOST_CHECK(found->endDate.has_value());
    BOOST_CHECK(found->isArchive.has_value());
    BOOST_CHECK(!found->isArchive.value());
}

BOOST_AUTO_TEST_CASE(test_create_phase_without_caption_fails)
{
    // Arrange
    dto::Phase phase;
    phase.projectId = m_testProjectId;

    // Act
    int64_t result = m_repository->create(phase);

    // Assert
    BOOST_CHECK_EQUAL(result, 0);
}

BOOST_AUTO_TEST_CASE(test_create_phase_without_project_fails)
{
    // Arrange
    auto phase = createTestPhase();
    phase.projectId = std::nullopt;

    // Act
    int64_t result = m_repository->create(phase);

    // Assert
    BOOST_CHECK_EQUAL(result, 0);
}

BOOST_AUTO_TEST_CASE(test_create_phase_search_fields_generated)
{
    // Arrange
    auto phase = createTestPhase("ФАЗА Для Поиска");
    phase.description = "ОПИСАНИЕ ФАЗЫ";

    // Act
    int64_t phaseId = m_repository->create(phase);

    // Assert
    auto conn = m_database->connection();
    auto stmt = conn->prepareStatement(
        "SELECT searchCaption, searchDescription FROM Phase WHERE id = :id"
    );
    stmt->bindInt64("id", phaseId);
    auto rs = stmt->executeQuery();

    BOOST_REQUIRE(rs->next());
    BOOST_CHECK_EQUAL(rs->valueString("searchCaption"), "фаза для поиска");
    BOOST_CHECK_EQUAL(rs->valueString("searchDescription"), "описание фазы");
}

BOOST_AUTO_TEST_CASE(test_create_phase_with_nullable_dates)
{
    // Arrange
    dto::Phase phase;
    phase.caption = "Фаза без дат";
    phase.projectId = m_testProjectId;
    // beginDate и endDate оставляем nullopt

    // Act
    int64_t phaseId = m_repository->create(phase);

    // Assert
    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->beginDate.has_value());
    BOOST_CHECK(!found->endDate.has_value());
}

// ============================================================
// Тесты поиска фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    // Arrange
    auto phase = createTestPhase("Фаза для поиска");
    int64_t phaseId = m_repository->create(phase);
    BOOST_REQUIRE_GT(phaseId, 0);

    // Act
    auto found = m_repository->findById(phaseId);

    // Assert
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found->id.value(), phaseId);
    BOOST_CHECK_EQUAL(found->caption.value(), "Фаза для поиска");
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    // Act
    auto found = m_repository->findById(99999);

    // Assert
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_id_invalid_returns_nullopt)
{
    // Act
    auto found = m_repository->findById(-1);

    // Assert
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты получения списка фаз с пагинацией
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_empty)
{
    // Act
    auto [phases, total] = m_repository->findAll(1, 20);

    // Assert
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(phases.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_with_pagination)
{
    // Arrange - создаем 12 фаз
    for (int i = 1; i <= 12; ++i)
    {
        m_repository->create(createTestPhase("Фаза " + std::to_string(i)));
    }

    // Act - первая страница
    auto [page1, total] = m_repository->findAll(1, 10);

    // Assert
    BOOST_CHECK_EQUAL(total, 12);
    BOOST_CHECK_EQUAL(page1.size(), 10);

    // Act - вторая страница
    auto [page2, total2] = m_repository->findAll(2, 10);
    BOOST_CHECK_EQUAL(page2.size(), 2);
    BOOST_CHECK_EQUAL(total2, 12);
}

BOOST_AUTO_TEST_CASE(test_find_all_by_project)
{
    // Arrange
    // Создаем еще один проект
    auto conn = m_database->connection();
    conn->execute(R"(
        INSERT INTO Project (caption, createdAt, updatedAt)
        VALUES ('Второй проект', strftime('%s', 'now'), strftime('%s', 'now'))
    )");
    int64_t secondProjectId = conn->lastInsertId();

    // Создаем фазы для первого проекта
    m_repository->create(createTestPhase("Фаза 1 проект 1"));
    m_repository->create(createTestPhase("Фаза 2 проект 1"));

    // Создаем фазы для второго проекта
    m_repository->create(createTestPhase("Фаза 1 проект 2", secondProjectId));

    // Act - фильтр по первому проекту
    auto [phases, total] = m_repository->findAll(1, 20, m_testProjectId);

    // Assert
    BOOST_CHECK_EQUAL(total, 2);
    for (const auto& phase : phases)
    {
        BOOST_CHECK_EQUAL(phase.projectId.value(), m_testProjectId);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_archived_filter)
{
    // Arrange
    auto activePhase = createTestPhase("Активная фаза");
    int64_t activeId = m_repository->create(activePhase);

    auto archivedPhase = createTestPhase("Архивная фаза");
    int64_t archivedId = m_repository->create(archivedPhase);
    m_repository->archive(archivedId);

    // Act - только активные
    auto [active, totalActive] = m_repository->findAll(1, 20, std::nullopt, false);
    BOOST_CHECK_EQUAL(totalActive, 1);
    BOOST_CHECK_EQUAL(active[0].id.value(), activeId);

    // Act - только архивные
    auto [archived, totalArchived] = m_repository->findAll(1, 20, std::nullopt, true);
    BOOST_CHECK_EQUAL(totalArchived, 1);
    BOOST_CHECK_EQUAL(archived[0].id.value(), archivedId);
}

// ============================================================
// Тесты обновления фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_phase_success)
{
    // Arrange
    auto phase = createTestPhase("Старая фаза");
    int64_t phaseId = m_repository->create(phase);

    // Act
    dto::Phase updateData;
    updateData.id = phaseId;
    updateData.caption = "Новая фаза";
    updateData.description = "Новое описание";

    bool result = m_repository->update(updateData);

    // Assert
    BOOST_CHECK(result);

    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found->caption.value(), "Новая фаза");
    BOOST_CHECK_EQUAL(found->description.value(), "Новое описание");
}

BOOST_AUTO_TEST_CASE(test_update_phase_dates)
{
    // Arrange
    auto phase = createTestPhase("Фаза с датами");
    int64_t phaseId = m_repository->create(phase);

    auto newBeginDate = std::chrono::system_clock::now() + std::chrono::hours(24);
    auto newEndDate = std::chrono::system_clock::now() + std::chrono::hours(48);

    // Act
    dto::Phase updateData;
    updateData.id = phaseId;
    updateData.beginDate = newBeginDate;
    updateData.endDate = newEndDate;

    bool result = m_repository->update(updateData);

    // Assert
    BOOST_CHECK(result);

    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->beginDate.has_value());
    BOOST_CHECK(found->endDate.has_value());
}

BOOST_AUTO_TEST_CASE(test_update_phase_partial)
{
    // Arrange
    auto phase = createTestPhase("Оригинальная фаза");
    phase.description = "Оригинальное описание";
    int64_t phaseId = m_repository->create(phase);

    // Act - обновляем только caption
    dto::Phase updateData;
    updateData.id = phaseId;
    updateData.caption = "Обновленная фаза";

    m_repository->update(updateData);

    // Assert
    auto found = m_repository->findById(phaseId);
    BOOST_CHECK_EQUAL(found->caption.value(), "Обновленная фаза");
    BOOST_CHECK_EQUAL(found->description.value(), "Оригинальное описание");
}

BOOST_AUTO_TEST_CASE(test_update_phase_search_fields_updated)
{
    // Arrange
    auto phase = createTestPhase("Старая ФАЗА");
    int64_t phaseId = m_repository->create(phase);

    // Act
    dto::Phase updateData;
    updateData.id = phaseId;
    updateData.caption = "НОВАЯ Фаза";

    m_repository->update(updateData);

    // Assert
    auto conn = m_database->connection();
    auto stmt = conn->prepareStatement(
        "SELECT searchCaption FROM Phase WHERE id = :id"
    );
    stmt->bindInt64("id", phaseId);
    auto rs = stmt->executeQuery();
    rs->next();
    BOOST_CHECK_EQUAL(rs->valueString("searchCaption"), "новая фаза");
}

BOOST_AUTO_TEST_CASE(test_update_phase_nonexistent)
{
    // Arrange
    dto::Phase updateData;
    updateData.id = 99999;
    updateData.caption = "Несуществующая фаза";

    // Act
    bool result = m_repository->update(updateData);

    // Assert
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты архивации и восстановления
// ============================================================

BOOST_AUTO_TEST_CASE(test_archive_phase_success)
{
    // Arrange
    auto phase = createTestPhase("Фаза для архивации");
    int64_t phaseId = m_repository->create(phase);

    // Act
    bool result = m_repository->archive(phaseId);

    // Assert
    BOOST_CHECK(result);

    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(found->isArchive.value());
}

BOOST_AUTO_TEST_CASE(test_restore_phase_success)
{
    // Arrange
    auto phase = createTestPhase("Фаза для восстановления");
    int64_t phaseId = m_repository->create(phase);
    m_repository->archive(phaseId);

    // Act
    bool result = m_repository->restore(phaseId);

    // Assert
    BOOST_CHECK(result);

    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK(!found->isArchive.value());
}

BOOST_AUTO_TEST_CASE(test_archive_nonexistent_phase)
{
    // Act
    bool result = m_repository->archive(99999);

    // Assert
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты вспомогательных методов
// ============================================================

BOOST_AUTO_TEST_CASE(test_exists_true)
{
    // Arrange
    int64_t phaseId = m_repository->create(createTestPhase());

    // Act
    bool exists = m_repository->exists(phaseId);

    // Assert
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_CASE(test_exists_false)
{
    // Act
    bool exists = m_repository->exists(99999);

    // Assert
    BOOST_CHECK(!exists);
}

BOOST_AUTO_TEST_CASE(test_find_by_project_all)
{
    // Arrange
    m_repository->create(createTestPhase("Фаза 1"));
    m_repository->create(createTestPhase("Фаза 2"));
    m_repository->create(createTestPhase("Фаза 3"));

    // Act
    auto phases = m_repository->findByProject(m_testProjectId);

    // Assert
    BOOST_CHECK_EQUAL(phases.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_find_by_project_exclude_archived)
{
    // Arrange
    m_repository->create(createTestPhase("Активная 1"));
    m_repository->create(createTestPhase("Активная 2"));

    auto archivedPhase = createTestPhase("Архивная");
    int64_t archivedId = m_repository->create(archivedPhase);
    m_repository->archive(archivedId);

    // Act - без архивных
    auto activePhases = m_repository->findByProject(m_testProjectId, false);
    BOOST_CHECK_EQUAL(activePhases.size(), 2);

    // Act - с архивными
    auto allPhases = m_repository->findByProject(m_testProjectId, true);
    BOOST_CHECK_EQUAL(allPhases.size(), 3);
}

BOOST_AUTO_TEST_CASE(test_find_by_project_invalid_id)
{
    // Act
    auto phases = m_repository->findByProject(-1);

    // Assert
    BOOST_CHECK(phases.empty());
}

// ============================================================
// Тесты сортировки
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_sorted_by_date)
{
    // Arrange - создаем фазы с разными датами начала
    auto now = std::chrono::system_clock::now();

    auto phase1 = createTestPhase("Первая фаза");
    phase1.beginDate = now;
    m_repository->create(phase1);

    auto phase2 = createTestPhase("Вторая фаза");
    phase2.beginDate = now + std::chrono::hours(24);
    m_repository->create(phase2);

    auto phase3 = createTestPhase("Третья фаза");
    phase3.beginDate = now - std::chrono::hours(24);
    m_repository->create(phase3);

    // Act
    auto [phases, total] = m_repository->findAll(1, 20);

    // Assert - должны быть отсортированы по beginDate
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(phases[0].caption.value(), "Третья фаза"); // Самая ранняя
    BOOST_CHECK_EQUAL(phases[1].caption.value(), "Первая фаза");
    BOOST_CHECK_EQUAL(phases[2].caption.value(), "Вторая фаза"); // Самая поздняя
}

// ============================================================
// Интеграционный тест: полный жизненный цикл фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_phase_lifecycle)
{
    // 1. Создание
    auto phase = createTestPhase("Жизненный цикл фазы");
    int64_t phaseId = m_repository->create(phase);
    BOOST_CHECK_GT(phaseId, 0);

    // 2. Чтение
    auto found = m_repository->findById(phaseId);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(found->caption.value(), "Жизненный цикл фазы");

    // 3. Обновление
    dto::Phase updateData;
    updateData.id = phaseId;
    updateData.caption = "Обновленная фаза";
    updateData.description = "Обновленное описание";
    BOOST_CHECK(m_repository->update(updateData));

    // 4. Проверка обновления
    found = m_repository->findById(phaseId);
    BOOST_CHECK_EQUAL(found->caption.value(), "Обновленная фаза");
    BOOST_CHECK_EQUAL(found->description.value(), "Обновленное описание");

    // 5. Архивация
    BOOST_CHECK(m_repository->archive(phaseId));
    found = m_repository->findById(phaseId);
    BOOST_CHECK(found->isArchive.value());

    // 6. Восстановление
    BOOST_CHECK(m_repository->restore(phaseId));
    found = m_repository->findById(phaseId);
    BOOST_CHECK(!found->isArchive.value());

    // 7. Проверка в списке проекта
    auto phases = m_repository->findByProject(m_testProjectId);
    BOOST_CHECK_EQUAL(phases.size(), 1);
    BOOST_CHECK_EQUAL(phases[0].id.value(), phaseId);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
