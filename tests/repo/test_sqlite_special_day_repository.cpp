#include <cstdio>
#include <filesystem>
#include <iostream>

#include <boost/test/unit_test.hpp>

#include "common/dto/special_day.h"
#include "common/log/log.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_special_day_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct SpecialDayRepositoryFixture
{
    SpecialDayRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_special_day_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для SpecialDay
        conn->execute(R"(
            CREATE TABLE SpecialDay (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                date INTEGER NOT NULL UNIQUE,
                isWorkDay INTEGER NOT NULL,
                beginWorkTime TEXT,
                endWorkTime TEXT,
                breakDuration INTEGER DEFAULT 0
            )
        )");

        // Добавляем тестовые данные
        addSpecialDay(conn, "2024-01-01", false);
        addSpecialDay(conn, "2024-03-08", true, "09:00", "15:00", 30);
        addSpecialDay(conn, "2024-05-01", false);
        addSpecialDay(conn, "2024-05-09", false);
        addSpecialDay(conn, "2024-06-12", false);
        addSpecialDay(conn, "2023-12-31", false);
        addSpecialDay(conn, "2023-05-15", false);

        m_repository = std::make_unique<repositories::SqliteSpecialDayRepository>(m_database);
    }

    void addSpecialDay(
        std::shared_ptr<db::IConnection> conn,
        const std::string& dateStr,
        bool isWorkDay,
        const std::string& beginWorkTime = "",
        const std::string& endWorkTime = "",
        int breakDuration = 0
    )
    {
        try
        {
            auto date = common::stringToDateTime(dateStr + " 00:00:00");

            auto stmt = conn->prepareStatement(
                "INSERT INTO SpecialDay (date, isWorkDay, beginWorkTime, endWorkTime, breakDuration) "
                "VALUES (:date, :isWorkDay, :beginWorkTime, :endWorkTime, :breakDuration)"
            );

            stmt->bindInt64("date", common::timePointToSeconds(date));
            stmt->bindInt64("isWorkDay", isWorkDay ? 1 : 0);

            if (!beginWorkTime.empty())
                stmt->bindString("beginWorkTime", beginWorkTime);
            else
                stmt->bindNull("beginWorkTime");

            if (!endWorkTime.empty())
                stmt->bindString("endWorkTime", endWorkTime);
            else
                stmt->bindNull("endWorkTime");

            stmt->bindInt64("breakDuration", breakDuration);
            stmt->execute();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to add special day " << dateStr << ": " << e.what() << std::endl;
        }
    }

    ~SpecialDayRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteSpecialDayRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteSpecialDayRepositoryTests, SpecialDayRepositoryFixture)

// ============================================================
// Тесты findAll
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_all_days)
{
    auto [days, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_GT(total, 0);
    BOOST_CHECK_EQUAL(days.size(), total);
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination)
{
    auto [page1, total] = m_repository->findAll(1, 2);
    BOOST_CHECK_GT(total, 0);
    BOOST_CHECK_LE(page1.size(), 2);

    if (total > 2)
    {
        auto [page2, total2] = m_repository->findAll(2, 2);
        BOOST_CHECK_GT(total2, 0);
        BOOST_CHECK_LE(page2.size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_year)
{
    auto [days, total] = m_repository->findAll(1, 20, 2024);
    BOOST_CHECK_GT(total, 0);
    BOOST_CHECK_EQUAL(days.size(), total);

    for (const auto& day : days)
    {
        auto tt = std::chrono::system_clock::to_time_t(*day.date);
        std::tm tm = *std::localtime(&tt);
        BOOST_CHECK_EQUAL(tm.tm_year + 1900, 2024);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_year_and_month)
{
    auto [days, total] = m_repository->findAll(1, 20, 2024, 5);
    BOOST_CHECK_GT(total, 0);
    BOOST_CHECK_EQUAL(days.size(), total);

    for (const auto& day : days)
    {
        auto tt = std::chrono::system_clock::to_time_t(*day.date);
        std::tm tm = *std::localtime(&tt);
        BOOST_CHECK_EQUAL(tm.tm_year + 1900, 2024);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1, 5);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_month_without_year)
{
    auto [days, total] = m_repository->findAll(1, 20, std::nullopt, 5);
    BOOST_CHECK_GT(total, 0);
    BOOST_CHECK_EQUAL(days.size(), total);

    for (const auto& day : days)
    {
        auto tt = std::chrono::system_clock::to_time_t(*day.date);
        std::tm tm = *std::localtime(&tt);
        BOOST_CHECK_EQUAL(tm.tm_mon + 1, 5);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_year_without_matches)
{
    auto [days, total] = m_repository->findAll(1, 20, 2025);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(days.empty());
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_year_and_month_without_matches)
{
    auto [days, total] = m_repository->findAll(1, 20, 2024, 12);
    BOOST_CHECK_EQUAL(total, 0);
    BOOST_CHECK(days.empty());
}

// ============================================================
// Тесты findById
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    auto [days, total] = m_repository->findAll(1, 20);
    BOOST_REQUIRE_GT(total, 0);
    int64_t id = *days[0].id;

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->id, id);
}

BOOST_AUTO_TEST_CASE(test_find_by_id_not_found)
{
    auto found = m_repository->findById(99999);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты findByDate
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_date_success)
{
    auto date = common::stringToDateTime("2024-01-01 00:00:00");
    auto found = m_repository->findByDate(date);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isWorkDay, false);
}

BOOST_AUTO_TEST_CASE(test_find_by_date_with_work_day)
{
    auto date = common::stringToDateTime("2024-03-08 00:00:00");
    auto found = m_repository->findByDate(date);
    if (found.has_value())
    {
        BOOST_CHECK_EQUAL(*found->isWorkDay, true);
        if (found->beginWorkTime.has_value())
        {
            BOOST_CHECK_EQUAL(*found->beginWorkTime, "09:00");
        }
        if (found->endWorkTime.has_value())
        {
            BOOST_CHECK_EQUAL(*found->endWorkTime, "15:00");
        }
        if (found->breakDuration.has_value())
        {
            BOOST_CHECK_EQUAL(*found->breakDuration, 30);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_date_not_found)
{
    auto date = common::stringToDateTime("2024-02-29 00:00:00");
    auto found = m_repository->findByDate(date);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты create
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_special_day_success)
{
    dto::SpecialDay newDay;
    newDay.date = common::stringToDateTime("2024-11-04 00:00:00");
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isWorkDay, false);
}

BOOST_AUTO_TEST_CASE(test_create_special_day_with_custom_time)
{
    dto::SpecialDay newDay;
    newDay.date = common::stringToDateTime("2024-12-31 00:00:00");
    newDay.isWorkDay = true;
    newDay.beginWorkTime = "09:00";
    newDay.endWorkTime = "14:00";
    newDay.breakDuration = 30;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isWorkDay, true);
    if (found->beginWorkTime.has_value())
    {
        BOOST_CHECK_EQUAL(*found->beginWorkTime, "09:00");
    }
    if (found->endWorkTime.has_value())
    {
        BOOST_CHECK_EQUAL(*found->endWorkTime, "14:00");
    }
    if (found->breakDuration.has_value())
    {
        BOOST_CHECK_EQUAL(*found->breakDuration, 30);
    }
}

BOOST_AUTO_TEST_CASE(test_create_special_day_duplicate_date_fails)
{
    dto::SpecialDay newDay;
    newDay.date = common::stringToDateTime("2024-01-01 00:00:00");
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_special_day_missing_date_fails)
{
    dto::SpecialDay newDay;
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_EQUAL(id, 0);
}

// ============================================================
// Тесты update
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_special_day_success)
{
    auto date = common::stringToDateTime("2024-05-01 00:00:00");
    auto existing = m_repository->findByDate(date);
    BOOST_REQUIRE(existing.has_value());

    dto::SpecialDay updateDay;
    updateDay.id = existing->id;
    updateDay.isWorkDay = true;
    updateDay.beginWorkTime = "10:00";
    updateDay.endWorkTime = "16:00";
    updateDay.breakDuration = 45;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto updated = m_repository->findById(*existing->id);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->isWorkDay, true);
    if (updated->beginWorkTime.has_value())
    {
        BOOST_CHECK_EQUAL(*updated->beginWorkTime, "10:00");
    }
    if (updated->endWorkTime.has_value())
    {
        BOOST_CHECK_EQUAL(*updated->endWorkTime, "16:00");
    }
    if (updated->breakDuration.has_value())
    {
        BOOST_CHECK_EQUAL(*updated->breakDuration, 45);
    }
}

BOOST_AUTO_TEST_CASE(test_update_special_day_missing_id_fails)
{
    dto::SpecialDay updateDay;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_special_day_nonexistent_id_fails)
{
    dto::SpecialDay updateDay;
    updateDay.id = 99999;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты remove
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_special_day_success)
{
    auto date = common::stringToDateTime("2024-06-12 00:00:00");
    auto existing = m_repository->findByDate(date);
    BOOST_REQUIRE(existing.has_value());

    bool result = m_repository->remove(*existing->id);
    BOOST_CHECK(result);

    auto found = m_repository->findById(*existing->id);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_remove_special_day_nonexistent_fails)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_special_day_lifecycle)
{
    // 1. Создание
    dto::SpecialDay newDay;
    newDay.date = common::stringToDateTime("2024-10-01 00:00:00");
    newDay.isWorkDay = true;
    newDay.beginWorkTime = "09:00";
    newDay.endWorkTime = "18:00";
    newDay.breakDuration = 60;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isWorkDay, true);

    // 3. Обновление
    dto::SpecialDay updateDay;
    updateDay.id = id;
    updateDay.isWorkDay = false;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto updated = m_repository->findById(id);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->isWorkDay, false);

    // 4. Поиск по дате
    auto byDate = m_repository->findByDate(newDay.date.value());
    BOOST_REQUIRE(byDate.has_value());
    BOOST_CHECK_EQUAL(*byDate->id, id);

    // 5. Удаление
    result = m_repository->remove(id);
    BOOST_CHECK(result);

    auto deleted = m_repository->findById(id);
    BOOST_CHECK(!deleted.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
