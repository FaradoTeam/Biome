#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/user_day.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(UserDayTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    UserDay dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.userId.has_value());
    BOOST_TEST(!dto.date.has_value());
    BOOST_TEST(!dto.isWorkDay.has_value());
    BOOST_TEST(!dto.beginWorkTime.has_value());
    BOOST_TEST(!dto.endWorkTime.has_value());
    BOOST_TEST(!dto.breakDuration.has_value());
    BOOST_TEST(!dto.description.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    UserDay dto;

    // Проверка поля: id
    {
        BOOST_TEST(!dto.id.has_value());

        int64_t testValue =42;
        dto.id = testValue;

        BOOST_TEST(dto.id.has_value());

        BOOST_TEST(dto.id.value() == testValue);

        // Проверка сброса значения
        dto.id = std::nullopt;
        BOOST_TEST(!dto.id.has_value());
    }
    // Проверка поля: userId
    {
        BOOST_TEST(!dto.userId.has_value());

        int64_t testValue =42;
        dto.userId = testValue;

        BOOST_TEST(dto.userId.has_value());

        BOOST_TEST(dto.userId.value() == testValue);

        // Проверка сброса значения
        dto.userId = std::nullopt;
        BOOST_TEST(!dto.userId.has_value());
    }
    // Проверка поля: date
    {
        BOOST_TEST(!dto.date.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.date = testValue;

        BOOST_TEST(dto.date.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.date.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.date = std::nullopt;
        BOOST_TEST(!dto.date.has_value());
    }
    // Проверка поля: isWorkDay
    {
        BOOST_TEST(!dto.isWorkDay.has_value());

        bool testValue =true;
        dto.isWorkDay = testValue;

        BOOST_TEST(dto.isWorkDay.has_value());

        BOOST_TEST(dto.isWorkDay.value() == testValue);

        // Проверка сброса значения
        dto.isWorkDay = std::nullopt;
        BOOST_TEST(!dto.isWorkDay.has_value());
    }
    // Проверка поля: beginWorkTime
    {
        BOOST_TEST(!dto.beginWorkTime.has_value());

        std::string testValue ="test_value";
        dto.beginWorkTime = testValue;

        BOOST_TEST(dto.beginWorkTime.has_value());

        BOOST_TEST(dto.beginWorkTime.value() == testValue);

        // Проверка сброса значения
        dto.beginWorkTime = std::nullopt;
        BOOST_TEST(!dto.beginWorkTime.has_value());
    }
    // Проверка поля: endWorkTime
    {
        BOOST_TEST(!dto.endWorkTime.has_value());

        std::string testValue ="test_value";
        dto.endWorkTime = testValue;

        BOOST_TEST(dto.endWorkTime.has_value());

        BOOST_TEST(dto.endWorkTime.value() == testValue);

        // Проверка сброса значения
        dto.endWorkTime = std::nullopt;
        BOOST_TEST(!dto.endWorkTime.has_value());
    }
    // Проверка поля: breakDuration
    {
        BOOST_TEST(!dto.breakDuration.has_value());

        int64_t testValue =42;
        dto.breakDuration = testValue;

        BOOST_TEST(dto.breakDuration.has_value());

        BOOST_TEST(dto.breakDuration.value() == testValue);

        // Проверка сброса значения
        dto.breakDuration = std::nullopt;
        BOOST_TEST(!dto.breakDuration.has_value());
    }
    // Проверка поля: description
    {
        BOOST_TEST(!dto.description.has_value());

        std::string testValue ="test_value";
        dto.description = testValue;

        BOOST_TEST(dto.description.has_value());

        BOOST_TEST(dto.description.value() == testValue);

        // Проверка сброса значения
        dto.description = std::nullopt;
        BOOST_TEST(!dto.description.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    UserDay dto;

    // Поле: id
    dto.id = 42;
    // Поле: userId
    dto.userId = 42;
    // Поле: date
    dto.date = secondsToTimePoint(1640995200);
    // Поле: isWorkDay
    dto.isWorkDay = true;
    // Поле: beginWorkTime
    dto.beginWorkTime = "test_begin_work_time";
    // Поле: endWorkTime
    dto.endWorkTime = "test_end_work_time";
    // Поле: breakDuration
    dto.breakDuration = 42;
    // Поле: description
    dto.description = "test_description";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("user_id"));
    BOOST_TEST(json["user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("date"));
    BOOST_TEST(json["date"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("is_work_day"));
    BOOST_TEST(json["is_work_day"].get<bool>() == true);
    BOOST_TEST(json.contains("begin_work_time"));
    BOOST_TEST(json["begin_work_time"].get<std::string>() == "test_begin_work_time");
    BOOST_TEST(json.contains("end_work_time"));
    BOOST_TEST(json["end_work_time"].get<std::string>() == "test_end_work_time");
    BOOST_TEST(json.contains("break_duration"));
    BOOST_TEST(json["break_duration"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["user_id"] = 42;
    json["date"] = 1640995200;
    json["is_work_day"] = true;
    json["begin_work_time"] = "test_begin_work_time";
    json["end_work_time"] = "test_end_work_time";
    json["break_duration"] = 42;
    json["description"] = "test_description";

    UserDay dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.userId.has_value());
    BOOST_TEST(dto.userId.value() == 42);
    BOOST_TEST(dto.date.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.date.value()), 1640995200);
    BOOST_TEST(dto.isWorkDay.has_value());
    BOOST_TEST(dto.isWorkDay.value() == true);
    BOOST_TEST(dto.beginWorkTime.has_value());
    BOOST_TEST(dto.beginWorkTime.value() == "test_begin_work_time");
    BOOST_TEST(dto.endWorkTime.has_value());
    BOOST_TEST(dto.endWorkTime.value() == "test_end_work_time");
    BOOST_TEST(dto.breakDuration.has_value());
    BOOST_TEST(dto.breakDuration.value() == 42);
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    UserDay original;

    // Поле: id
    original.id = 42;
    // Поле: userId
    original.userId = 42;
    // Поле: date
    original.date = secondsToTimePoint(1640995200);
    // Поле: isWorkDay
    original.isWorkDay = true;
    // Поле: beginWorkTime
    original.beginWorkTime = "test_begin_work_time";
    // Поле: endWorkTime
    original.endWorkTime = "test_end_work_time";
    // Поле: breakDuration
    original.breakDuration = 42;
    // Поле: description
    original.description = "test_description";

    nlohmann::json json = original.toJson();
    UserDay deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    UserDay dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.userId = 42;
    dto.date = secondsToTimePoint(1640995200);

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    UserDay dto1;
    UserDay dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле userId, чтобы сделать их разными
    dto1.userId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    UserDay dto;

    dto.userId = 42;
    dto.date = secondsToTimePoint(1640995200);

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()