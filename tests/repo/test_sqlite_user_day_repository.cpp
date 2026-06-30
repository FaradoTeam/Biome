#include <cstdio>
#include <filesystem>
#include <thread>

#include <boost/test/unit_test.hpp>

#include "common/dto/user_day.h"
#include "common/types.h"

#include "repo/sqlite/sqlite_user_day_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct UserDayRepositoryFixture
{
    UserDayRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_user_day_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для UserDay
        conn->execute(R"(
            CREATE TABLE User (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                login TEXT NOT NULL UNIQUE,
                email TEXT NOT NULL UNIQUE,
                passwordHash TEXT NOT NULL,
                needChangePassword INTEGER NOT NULL DEFAULT 1,
                isBlocked INTEGER NOT NULL DEFAULT 0,
                isSuperAdmin INTEGER NOT NULL DEFAULT 0,
                isHidden INTEGER NOT NULL DEFAULT 0
            )
        )");

        conn->execute(R"(
            CREATE TABLE UserDay (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                userId INTEGER NOT NULL,
                date INTEGER NOT NULL,
                isWorkDay INTEGER NOT NULL,
                beginWorkTime TEXT,
                endWorkTime TEXT,
                breakDuration INTEGER DEFAULT 0,
                description TEXT,
                FOREIGN KEY (userId) REFERENCES User(id) ON DELETE CASCADE,
                UNIQUE(userId, date)
            )
        )");

        // Создаем тестовых пользователей
        for (int i = 1; i <= 3; ++i)
        {
            conn->execute(
                "INSERT INTO User (login, email, passwordHash) "
                "VALUES ('user"
                + std::to_string(i) + "', 'user" + std::to_string(i) + "@mail.local', 'hash')"
            );
        }

        // Получаем ID пользователей
        auto stmt = conn->prepareStatement("SELECT id FROM User ORDER BY id");
        auto rs = stmt->executeQuery();
        while (rs->next())
        {
            m_userIds.push_back(rs->valueInt64(0));
        }

        // Добавляем тестовые дни для пользователей
        // Пользователь 1: отпуск 1-10 января
        addUserDay(conn, m_userIds[0], "2024-01-01", false, std::nullopt, std::nullopt, 0, "Новогодние каникулы");
        addUserDay(conn, m_userIds[0], "2024-01-02", false, std::nullopt, std::nullopt, 0, "Новогодние каникулы");
        addUserDay(conn, m_userIds[0], "2024-01-03", false, std::nullopt, std::nullopt, 0, "Новогодние каникулы");

        // Пользователь 2: больничный
        addUserDay(conn, m_userIds[1], "2024-02-15", false, std::nullopt, std::nullopt, 0, "Больничный");
        addUserDay(conn, m_userIds[1], "2024-02-16", false, std::nullopt, std::nullopt, 0, "Больничный");

        // Пользователь 3: дополнительный рабочий день (суббота)
        addUserDay(conn, m_userIds[2], "2024-03-23", true, std::optional<std::string>("09:00"), std::optional<std::string>("14:00"), 30, "Дополнительный рабочий день");

        m_repository = std::make_unique<repositories::SqliteUserDayRepository>(m_database);
    }

    void addUserDay(
        std::shared_ptr<db::IConnection> conn,
        int64_t userId,
        const std::string& dateStr,
        bool isWorkDay,
        std::optional<std::string> beginWorkTime,
        std::optional<std::string> endWorkTime,
        int breakDuration,
        const std::string& description
    )
    {
        auto date = common::stringToDateTime(dateStr + " 00:00:00");
        auto stmt = conn->prepareStatement(
            "INSERT INTO UserDay (userId, date, isWorkDay, beginWorkTime, endWorkTime, "
            "breakDuration, description) "
            "VALUES (:userId, :date, :isWorkDay, :beginWorkTime, :endWorkTime, "
            ":breakDuration, :description)"
        );

        stmt->bindInt64("userId", userId);
        stmt->bindInt64("date", common::timePointToSeconds(date));
        stmt->bindInt64("isWorkDay", isWorkDay ? 1 : 0);

        if (beginWorkTime.has_value())
            stmt->bindString("beginWorkTime", *beginWorkTime);
        else
            stmt->bindNull("beginWorkTime");

        if (endWorkTime.has_value())
            stmt->bindString("endWorkTime", *endWorkTime);
        else
            stmt->bindNull("endWorkTime");

        stmt->bindInt64("breakDuration", breakDuration);
        stmt->bindString("description", description);
        stmt->execute();
    }

    ~UserDayRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteUserDayRepository> m_repository;
    std::vector<int64_t> m_userIds;
};

BOOST_FIXTURE_TEST_SUITE(SqliteUserDayRepositoryTests, UserDayRepositoryFixture)

// ============================================================
// Тесты findAll
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_all_days)
{
    auto [days, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_EQUAL(total, 6); // 3 + 2 + 1 = 6
    BOOST_CHECK_EQUAL(days.size(), 6);
}

BOOST_AUTO_TEST_CASE(test_find_all_pagination)
{
    auto [page1, total] = m_repository->findAll(1, 2);
    BOOST_CHECK_EQUAL(total, 6);
    BOOST_CHECK_EQUAL(page1.size(), 2);

    auto [page2, total2] = m_repository->findAll(2, 2);
    BOOST_CHECK_EQUAL(total2, 6);
    BOOST_CHECK_EQUAL(page2.size(), 2);

    auto [page3, total3] = m_repository->findAll(3, 2);
    BOOST_CHECK_EQUAL(total3, 6);
    BOOST_CHECK_EQUAL(page3.size(), 2);

    auto [page4, total4] = m_repository->findAll(4, 2);
    BOOST_CHECK_EQUAL(total4, 6);
    BOOST_CHECK_EQUAL(page4.size(), 0);
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user)
{
    auto [days, total] = m_repository->findAll(1, 20, m_userIds[0]);
    BOOST_CHECK_EQUAL(total, 3);
    BOOST_CHECK_EQUAL(days.size(), 3);

    for (const auto& day : days)
    {
        BOOST_CHECK_EQUAL(*day.userId, m_userIds[0]);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user_and_date_range)
{
    auto dateFrom = common::stringToDateTime("2024-01-15 00:00:00");
    auto dateTo = common::stringToDateTime("2024-02-20 00:00:00");

    auto [days, total] = m_repository->findAll(1, 20, std::nullopt, dateFrom, dateTo);
    // Должны найтись только дни пользователя 2 (15-16 февраля)
    BOOST_CHECK_EQUAL(total, 2);
    BOOST_CHECK_EQUAL(days.size(), 2);

    for (const auto& day : days)
    {
        BOOST_CHECK_EQUAL(*day.userId, m_userIds[1]);
        auto tt = std::chrono::system_clock::to_time_t(*day.date);
        std::tm tm = *std::localtime(&tt);
        BOOST_CHECK(tm.tm_mon + 1 == 2 && tm.tm_mday >= 15 && tm.tm_mday <= 16);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_filter_by_user_and_date_range_with_exact)
{
    auto dateFrom = common::stringToDateTime("2024-03-01 00:00:00");
    auto dateTo = common::stringToDateTime("2024-03-31 00:00:00");

    auto [days, total] = m_repository->findAll(1, 20, m_userIds[2], dateFrom, dateTo);
    // Должен найтись только дополнительный рабочий день (23 марта)
    BOOST_CHECK_EQUAL(total, 1);
    BOOST_CHECK_EQUAL(days.size(), 1);
    BOOST_CHECK_EQUAL(*days[0].isWorkDay, true);
    BOOST_CHECK_EQUAL(*days[0].description, "Дополнительный рабочий день");
}

BOOST_AUTO_TEST_CASE(test_find_all_order_by_date_desc)
{
    auto [days, total] = m_repository->findAll(1, 20);
    BOOST_CHECK_GT(total, 0);

    // Проверяем, что дни отсортированы по убыванию даты
    for (size_t i = 0; i < days.size() - 1; ++i)
    {
        BOOST_CHECK(*days[i].date >= *days[i + 1].date);
    }
}

// ============================================================
// Тесты findById
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_id_success)
{
    // Получаем ID первого дня
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
// Тесты findByUserAndDate
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_user_and_date_success)
{
    auto date = common::stringToDateTime("2024-01-01 00:00:00");
    auto found = m_repository->findByUserAndDate(m_userIds[0], date);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_userIds[0]);
    BOOST_CHECK_EQUAL(*found->isWorkDay, false);
    BOOST_CHECK_EQUAL(*found->description, "Новогодние каникулы");
}

BOOST_AUTO_TEST_CASE(test_find_by_user_and_date_work_day)
{
    auto date = common::stringToDateTime("2024-03-23 00:00:00");
    auto found = m_repository->findByUserAndDate(m_userIds[2], date);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_userIds[2]);
    BOOST_CHECK_EQUAL(*found->isWorkDay, true);
    BOOST_CHECK_EQUAL(*found->beginWorkTime, "09:00");
    BOOST_CHECK_EQUAL(*found->endWorkTime, "14:00");
    BOOST_CHECK_EQUAL(*found->breakDuration, 30);
}

BOOST_AUTO_TEST_CASE(test_find_by_user_and_date_not_found)
{
    auto date = common::stringToDateTime("2024-04-01 00:00:00");
    auto found = m_repository->findByUserAndDate(m_userIds[0], date);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_user_and_date_invalid_user)
{
    auto date = common::stringToDateTime("2024-01-01 00:00:00");
    auto found = m_repository->findByUserAndDate(99999, date);
    BOOST_CHECK(!found.has_value());
}

// ============================================================
// Тесты findByUserId
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_user_id)
{
    auto days = m_repository->findByUserId(m_userIds[1]);
    BOOST_CHECK_EQUAL(days.size(), 2);

    for (const auto& day : days)
    {
        BOOST_CHECK_EQUAL(*day.userId, m_userIds[1]);
        BOOST_CHECK_EQUAL(*day.isWorkDay, false);
        BOOST_CHECK_EQUAL(*day.description, "Больничный");
    }
}

BOOST_AUTO_TEST_CASE(test_find_by_user_id_empty)
{
    auto days = m_repository->findByUserId(99999);
    BOOST_CHECK(days.empty());
}

// ============================================================
// Тесты create
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_user_day_success)
{
    dto::UserDay newDay;
    newDay.userId = m_userIds[0];
    newDay.date = common::stringToDateTime("2024-03-08 00:00:00");
    newDay.isWorkDay = false;
    newDay.description = "Отгул";

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_userIds[0]);
    BOOST_CHECK_EQUAL(*found->isWorkDay, false);
    BOOST_CHECK_EQUAL(*found->description, "Отгул");
}

BOOST_AUTO_TEST_CASE(test_create_user_day_with_custom_time)
{
    dto::UserDay newDay;
    newDay.userId = m_userIds[0];
    newDay.date = common::stringToDateTime("2024-04-12 00:00:00");
    newDay.isWorkDay = true;
    newDay.beginWorkTime = "08:00";
    newDay.endWorkTime = "16:00";
    newDay.breakDuration = 45;
    newDay.description = "Сокращённый день";

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->isWorkDay, true);
    BOOST_CHECK_EQUAL(*found->beginWorkTime, "08:00");
    BOOST_CHECK_EQUAL(*found->endWorkTime, "16:00");
    BOOST_CHECK_EQUAL(*found->breakDuration, 45);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_duplicate_fails)
{
    dto::UserDay newDay;
    newDay.userId = m_userIds[0];
    newDay.date = common::stringToDateTime("2024-01-01 00:00:00"); // Уже существует
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_missing_user_fails)
{
    dto::UserDay newDay;
    newDay.date = common::stringToDateTime("2024-01-10 00:00:00");
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_EQUAL(id, 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_missing_date_fails)
{
    dto::UserDay newDay;
    newDay.userId = m_userIds[0];
    newDay.isWorkDay = false;

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_EQUAL(id, 0);
}

// ============================================================
// Тесты update
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_user_day_success)
{
    auto date = common::stringToDateTime("2024-01-01 00:00:00");
    auto existing = m_repository->findByUserAndDate(m_userIds[0], date);
    BOOST_REQUIRE(existing.has_value());

    dto::UserDay updateDay;
    updateDay.id = existing->id;
    updateDay.isWorkDay = true; // Меняем на рабочий
    updateDay.beginWorkTime = "10:00";
    updateDay.endWorkTime = "15:00";
    updateDay.breakDuration = 30;
    updateDay.description = "Изменённый день";

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto updated = m_repository->findById(*existing->id);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->isWorkDay, true);
    BOOST_CHECK_EQUAL(*updated->beginWorkTime, "10:00");
    BOOST_CHECK_EQUAL(*updated->endWorkTime, "15:00");
    BOOST_CHECK_EQUAL(*updated->breakDuration, 30);
    BOOST_CHECK_EQUAL(*updated->description, "Изменённый день");
}

BOOST_AUTO_TEST_CASE(test_update_user_day_change_user_and_date)
{
    auto existing = m_repository->findByUserAndDate(m_userIds[1], common::stringToDateTime("2024-02-15 00:00:00"));
    BOOST_REQUIRE(existing.has_value());

    dto::UserDay updateDay;
    updateDay.id = existing->id;
    updateDay.userId = m_userIds[2];
    updateDay.date = common::stringToDateTime("2024-02-20 00:00:00");
    updateDay.isWorkDay = false;
    updateDay.description = "Перенесённый день";

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto updated = m_repository->findById(*existing->id);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->userId, m_userIds[2]);
    auto tt = std::chrono::system_clock::to_time_t(*updated->date);
    std::tm tm = *std::localtime(&tt);
    BOOST_CHECK_EQUAL(tm.tm_mday, 20);
    BOOST_CHECK_EQUAL(*updated->description, "Перенесённый день");
}

BOOST_AUTO_TEST_CASE(test_update_user_day_conflict_fails)
{
    auto existing = m_repository->findByUserAndDate(m_userIds[0], common::stringToDateTime("2024-01-01 00:00:00"));
    BOOST_REQUIRE(existing.has_value());

    dto::UserDay updateDay;
    updateDay.id = existing->id;
    updateDay.userId = m_userIds[0];
    updateDay.date = common::stringToDateTime("2024-01-02 00:00:00"); // Этот день уже существует
    updateDay.isWorkDay = false;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_user_day_missing_id_fails)
{
    dto::UserDay updateDay;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_user_day_nonexistent_id_fails)
{
    dto::UserDay updateDay;
    updateDay.id = 99999;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты remove
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_user_day_success)
{
    auto existing = m_repository->findByUserAndDate(m_userIds[0], common::stringToDateTime("2024-01-03 00:00:00"));
    BOOST_REQUIRE(existing.has_value());

    bool result = m_repository->remove(*existing->id);
    BOOST_CHECK(result);

    auto found = m_repository->findById(*existing->id);
    BOOST_CHECK(!found.has_value());
}

BOOST_AUTO_TEST_CASE(test_remove_user_day_nonexistent_fails)
{
    bool result = m_repository->remove(99999);
    BOOST_CHECK(!result);
}

// ============================================================
// Тесты removeByUserId
// ============================================================

BOOST_AUTO_TEST_CASE(test_remove_by_user_id)
{
    int64_t removed = m_repository->removeByUserId(m_userIds[1]);
    BOOST_CHECK_EQUAL(removed, 2);

    auto days = m_repository->findByUserId(m_userIds[1]);
    BOOST_CHECK(days.empty());
}

BOOST_AUTO_TEST_CASE(test_remove_by_user_id_invalid)
{
    int64_t removed = m_repository->removeByUserId(99999);
    BOOST_CHECK_EQUAL(removed, 0);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_user_day_lifecycle)
{
    // 1. Создание
    dto::UserDay newDay;
    newDay.userId = m_userIds[2];
    newDay.date = common::stringToDateTime("2024-05-10 00:00:00");
    newDay.isWorkDay = false;
    newDay.description = "Отпуск";

    int64_t id = m_repository->create(newDay);
    BOOST_CHECK_GT(id, 0);

    // 2. Чтение
    auto found = m_repository->findById(id);
    BOOST_REQUIRE(found.has_value());
    BOOST_CHECK_EQUAL(*found->userId, m_userIds[2]);
    BOOST_CHECK_EQUAL(*found->isWorkDay, false);

    // 3. Поиск по пользователю и дате
    auto byUserAndDate = m_repository->findByUserAndDate(m_userIds[2], *newDay.date);
    BOOST_REQUIRE(byUserAndDate.has_value());
    BOOST_CHECK_EQUAL(*byUserAndDate->id, id);

    // 4. Обновление
    dto::UserDay updateDay;
    updateDay.id = id;
    updateDay.isWorkDay = true;
    updateDay.beginWorkTime = "09:00";
    updateDay.endWorkTime = "13:00";
    updateDay.breakDuration = 0;
    updateDay.description = "Сокращённый день";

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto updated = m_repository->findById(id);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->isWorkDay, true);
    BOOST_CHECK_EQUAL(*updated->beginWorkTime, "09:00");
    BOOST_CHECK_EQUAL(*updated->endWorkTime, "13:00");

    // 5. Удаление
    result = m_repository->remove(id);
    BOOST_CHECK(result);

    auto deleted = m_repository->findById(id);
    BOOST_CHECK(!deleted.has_value());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
