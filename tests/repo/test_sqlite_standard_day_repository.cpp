#include <cstdio>
#include <filesystem>

#include <boost/test/unit_test.hpp>

#include "common/dto/standard_day.h"

#include "repo/sqlite/sqlite_standard_day_repository.h"
#include "storage/database_factory.h"
#include "storage/idatabase.h"
#include "storage/sqlite/sqlite_database.h"

namespace server::test
{

struct StandardDayRepositoryFixture
{
    StandardDayRepositoryFixture()
    {
        m_tempDbPath = std::filesystem::temp_directory_path() / "test_standard_day_repo.db";
        std::remove(m_tempDbPath.c_str());

        db::DatabaseConfig config;
        config["database"] = m_tempDbPath.string();

        m_database = std::make_shared<db::SqliteDatabase>();
        m_database->initialize(config);

        auto conn = m_database->connection();

        // Создаем схему для StandardDay
        conn->execute(R"(
            CREATE TABLE StandardDay (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                weekDayNumber INTEGER NOT NULL CHECK (weekDayNumber BETWEEN 0 AND 6),
                weekOrder INTEGER DEFAULT 0,
                isWorkDay INTEGER NOT NULL DEFAULT 1,
                beginWorkTime TEXT,
                endWorkTime TEXT,
                breakDuration INTEGER DEFAULT 0
            )
        )");

        // Добавляем тестовые данные (стандартная рабочая неделя)
        // Понедельник (1) - рабочий
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (1, 0, 1, '09:00', '18:00', 60)"
        );

        // Вторник (2) - рабочий
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (2, 0, 1, '09:00', '18:00', 60)"
        );

        // Среда (3) - рабочий
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (3, 0, 1, '09:00', '18:00', 60)"
        );

        // Четверг (4) - рабочий
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (4, 0, 1, '09:00', '18:00', 60)"
        );

        // Пятница (5) - рабочий
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (5, 0, 1, '09:00', '18:00', 60)"
        );

        // Суббота (6) - выходной
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (6, 0, 0, NULL, NULL, 0)"
        );

        // Воскресенье (0) - выходной
        conn->execute(
            "INSERT INTO StandardDay (weekDayNumber, weekOrder, isWorkDay, "
            "beginWorkTime, endWorkTime, breakDuration) "
            "VALUES (0, 0, 0, NULL, NULL, 0)"
        );

        m_repository = std::make_unique<repositories::SqliteStandardDayRepository>(m_database);
    }

    ~StandardDayRepositoryFixture()
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
    std::unique_ptr<repositories::SqliteStandardDayRepository> m_repository;
};

BOOST_FIXTURE_TEST_SUITE(SqliteStandardDayRepositoryTests, StandardDayRepositoryFixture)

// ============================================================
// Тесты findAll
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_all_returns_7_days)
{
    auto days = m_repository->findAll();
    BOOST_CHECK_EQUAL(days.size(), 7);
}

BOOST_AUTO_TEST_CASE(test_find_all_order_by_week_day)
{
    auto days = m_repository->findAll();

    // Проверяем, что дни отсортированы по weekDayNumber
    for (size_t i = 0; i < days.size() - 1; ++i)
    {
        BOOST_CHECK_LT(*days[i].weekDayNumber, *days[i + 1].weekDayNumber);
    }

    // Проверяем конкретные значения
    BOOST_CHECK_EQUAL(*days[0].weekDayNumber, 0); // Воскресенье
    BOOST_CHECK_EQUAL(*days[1].weekDayNumber, 1); // Понедельник
    BOOST_CHECK_EQUAL(*days[2].weekDayNumber, 2); // Вторник
    BOOST_CHECK_EQUAL(*days[3].weekDayNumber, 3); // Среда
    BOOST_CHECK_EQUAL(*days[4].weekDayNumber, 4); // Четверг
    BOOST_CHECK_EQUAL(*days[5].weekDayNumber, 5); // Пятница
    BOOST_CHECK_EQUAL(*days[6].weekDayNumber, 6); // Суббота
}

BOOST_AUTO_TEST_CASE(test_find_all_correct_work_days)
{
    auto days = m_repository->findAll();

    // Воскресенье и суббота - выходные
    BOOST_CHECK_EQUAL(*days[0].isWorkDay, false);
    BOOST_CHECK_EQUAL(*days[6].isWorkDay, false);

    // Понедельник-пятница - рабочие
    for (int i = 1; i <= 5; ++i)
    {
        BOOST_CHECK_EQUAL(*days[i].isWorkDay, true);
    }
}

BOOST_AUTO_TEST_CASE(test_find_all_correct_work_times)
{
    auto days = m_repository->findAll();

    // Проверяем рабочее время для будних дней
    for (int i = 1; i <= 5; ++i)
    {
        BOOST_CHECK_EQUAL(*days[i].beginWorkTime, "09:00");
        BOOST_CHECK_EQUAL(*days[i].endWorkTime, "18:00");
        BOOST_CHECK_EQUAL(*days[i].breakDuration, 60);
    }

    // Выходные дни не имеют времени работы
    BOOST_CHECK(!days[0].beginWorkTime.has_value());
    BOOST_CHECK(!days[0].endWorkTime.has_value());
    BOOST_CHECK_EQUAL(*days[0].breakDuration, 0);

    BOOST_CHECK(!days[6].beginWorkTime.has_value());
    BOOST_CHECK(!days[6].endWorkTime.has_value());
    BOOST_CHECK_EQUAL(*days[6].breakDuration, 0);
}

// ============================================================
// Тесты findByWeekDayNumber
// ============================================================

BOOST_AUTO_TEST_CASE(test_find_by_week_day_number_success)
{
    auto day = m_repository->findByWeekDayNumber(1); // Понедельник
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->weekDayNumber, 1);
    BOOST_CHECK_EQUAL(*day->isWorkDay, true);
    BOOST_CHECK_EQUAL(*day->beginWorkTime, "09:00");
    BOOST_CHECK_EQUAL(*day->endWorkTime, "18:00");
    BOOST_CHECK_EQUAL(*day->breakDuration, 60);
}

BOOST_AUTO_TEST_CASE(test_find_by_week_day_number_weekend)
{
    auto day = m_repository->findByWeekDayNumber(6); // Суббота
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->weekDayNumber, 6);
    BOOST_CHECK_EQUAL(*day->isWorkDay, false);
    BOOST_CHECK(!day->beginWorkTime.has_value());
    BOOST_CHECK(!day->endWorkTime.has_value());
    BOOST_CHECK_EQUAL(*day->breakDuration, 0);
}

BOOST_AUTO_TEST_CASE(test_find_by_week_day_number_invalid_range)
{
    auto day = m_repository->findByWeekDayNumber(-1);
    BOOST_CHECK(!day.has_value());

    day = m_repository->findByWeekDayNumber(7);
    BOOST_CHECK(!day.has_value());
}

BOOST_AUTO_TEST_CASE(test_find_by_week_day_number_not_found)
{
    // Удаляем запись для понедельника и проверяем, что её нет
    auto conn = m_database->connection();
    conn->execute("DELETE FROM StandardDay WHERE weekDayNumber = 1");

    auto day = m_repository->findByWeekDayNumber(1);
    BOOST_CHECK(!day.has_value());
}

// ============================================================
// Тесты update
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_work_day)
{
    // Меняем время работы для понедельника
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 1;
    updateDay.isWorkDay = true;
    updateDay.beginWorkTime = "08:00";
    updateDay.endWorkTime = "17:00";
    updateDay.breakDuration = 30;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto day = m_repository->findByWeekDayNumber(1);
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->beginWorkTime, "08:00");
    BOOST_CHECK_EQUAL(*day->endWorkTime, "17:00");
    BOOST_CHECK_EQUAL(*day->breakDuration, 30);
}

BOOST_AUTO_TEST_CASE(test_update_weekend_to_work_day)
{
    // Делаем субботу рабочим днём
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 6;
    updateDay.isWorkDay = true;
    updateDay.beginWorkTime = "09:00";
    updateDay.endWorkTime = "15:00";
    updateDay.breakDuration = 30;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto day = m_repository->findByWeekDayNumber(6);
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->isWorkDay, true);
    BOOST_CHECK_EQUAL(*day->beginWorkTime, "09:00");
    BOOST_CHECK_EQUAL(*day->endWorkTime, "15:00");
    BOOST_CHECK_EQUAL(*day->breakDuration, 30);
}

BOOST_AUTO_TEST_CASE(test_update_work_day_to_weekend)
{
    // Делаем пятницу выходным днём
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 5;
    updateDay.isWorkDay = false;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto day = m_repository->findByWeekDayNumber(5);
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->isWorkDay, false);
    // При выходном дне время должно быть NULL, а перерыв 0
    BOOST_CHECK(!day->beginWorkTime.has_value());
    BOOST_CHECK(!day->endWorkTime.has_value());
    BOOST_CHECK_EQUAL(*day->breakDuration, 0);
}

BOOST_AUTO_TEST_CASE(test_update_week_day_only)
{
    // Обновляем только статус работы, время не меняем
    // Сначала получаем текущие значения
    auto original = m_repository->findByWeekDayNumber(2);
    BOOST_REQUIRE(original.has_value());

    // Сохраняем оригинальные значения времени
    std::optional<std::string> originalBegin = original->beginWorkTime;
    std::optional<std::string> originalEnd = original->endWorkTime;
    int64_t originalBreak = *original->breakDuration;

    // Обновляем только статус работы на false (выходной)
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 2;
    updateDay.isWorkDay = false;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    auto day = m_repository->findByWeekDayNumber(2);
    BOOST_REQUIRE(day.has_value());
    BOOST_CHECK_EQUAL(*day->isWorkDay, false);

    // При выходном дне время должно быть NULL, а перерыв 0
    BOOST_CHECK(!day->beginWorkTime.has_value());
    BOOST_CHECK(!day->endWorkTime.has_value());
    BOOST_CHECK_EQUAL(*day->breakDuration, 0);

    // Возвращаем обратно рабочий день
    dto::StandardDay restoreDay;
    restoreDay.weekDayNumber = 2;
    restoreDay.isWorkDay = true;
    restoreDay.beginWorkTime = originalBegin;
    restoreDay.endWorkTime = originalEnd;
    restoreDay.breakDuration = originalBreak;

    result = m_repository->update(restoreDay);
    BOOST_CHECK(result);

    auto restored = m_repository->findByWeekDayNumber(2);
    BOOST_REQUIRE(restored.has_value());
    BOOST_CHECK_EQUAL(*restored->isWorkDay, true);
    if (originalBegin.has_value())
    {
        BOOST_CHECK(restored->beginWorkTime.has_value());
        BOOST_CHECK_EQUAL(*restored->beginWorkTime, *originalBegin);
    }
    else
    {
        BOOST_CHECK(!restored->beginWorkTime.has_value());
    }
    if (originalEnd.has_value())
    {
        BOOST_CHECK(restored->endWorkTime.has_value());
        BOOST_CHECK_EQUAL(*restored->endWorkTime, *originalEnd);
    }
    else
    {
        BOOST_CHECK(!restored->endWorkTime.has_value());
    }
    BOOST_CHECK_EQUAL(*restored->breakDuration, originalBreak);
}

BOOST_AUTO_TEST_CASE(test_update_nonexistent_week_day)
{
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 10;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

BOOST_AUTO_TEST_CASE(test_update_without_week_day_fails)
{
    dto::StandardDay updateDay;
    updateDay.isWorkDay = true;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(!result);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_standard_day_lifecycle)
{
    // 1. Получаем текущий день
    auto original = m_repository->findByWeekDayNumber(3);
    BOOST_REQUIRE(original.has_value());

    // 2. Изменяем день
    dto::StandardDay updateDay;
    updateDay.weekDayNumber = 3;
    updateDay.isWorkDay = false;

    bool result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    // 3. Проверяем изменение
    auto updated = m_repository->findByWeekDayNumber(3);
    BOOST_REQUIRE(updated.has_value());
    BOOST_CHECK_EQUAL(*updated->isWorkDay, false);

    // 4. Возвращаем обратно
    updateDay.isWorkDay = true;
    updateDay.beginWorkTime = "09:00";
    updateDay.endWorkTime = "18:00";
    updateDay.breakDuration = 60;

    result = m_repository->update(updateDay);
    BOOST_CHECK(result);

    // 5. Проверяем восстановление
    auto restored = m_repository->findByWeekDayNumber(3);
    BOOST_REQUIRE(restored.has_value());
    BOOST_CHECK_EQUAL(*restored->isWorkDay, true);
    BOOST_CHECK_EQUAL(*restored->beginWorkTime, "09:00");
    BOOST_CHECK_EQUAL(*restored->endWorkTime, "18:00");
    BOOST_CHECK_EQUAL(*restored->breakDuration, 60);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::test
