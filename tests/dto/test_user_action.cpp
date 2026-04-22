#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/user_action.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(UserActionTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    UserAction dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.userId.has_value());
    BOOST_TEST(!dto.timestamp.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    UserAction dto;

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
    // Проверка поля: timestamp
    {
        BOOST_TEST(!dto.timestamp.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.timestamp = testValue;

        BOOST_TEST(dto.timestamp.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.timestamp.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.timestamp = std::nullopt;
        BOOST_TEST(!dto.timestamp.has_value());
    }
    // Проверка поля: caption
    {
        BOOST_TEST(!dto.caption.has_value());

        std::string testValue ="test_value";
        dto.caption = testValue;

        BOOST_TEST(dto.caption.has_value());

        BOOST_TEST(dto.caption.value() == testValue);

        // Проверка сброса значения
        dto.caption = std::nullopt;
        BOOST_TEST(!dto.caption.has_value());
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
    UserAction dto;

    // Поле: id
    dto.id = 42;
    // Поле: userId
    dto.userId = 42;
    // Поле: timestamp
    dto.timestamp = secondsToTimePoint(1640995200);
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("user_id"));
    BOOST_TEST(json["user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("timestamp"));
    BOOST_TEST(json["timestamp"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["user_id"] = 42;
    json["timestamp"] = 1640995200;
    json["caption"] = "test_caption";
    json["description"] = "test_description";

    UserAction dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.userId.has_value());
    BOOST_TEST(dto.userId.value() == 42);
    BOOST_TEST(dto.timestamp.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.timestamp.value()), 1640995200);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    UserAction original;

    // Поле: id
    original.id = 42;
    // Поле: userId
    original.userId = 42;
    // Поле: timestamp
    original.timestamp = secondsToTimePoint(1640995200);
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";

    nlohmann::json json = original.toJson();
    UserAction deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    UserAction dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.userId = 42;
    dto.timestamp = secondsToTimePoint(1640995200);
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    UserAction dto1;
    UserAction dto2;

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
    UserAction dto;

    dto.userId = 42;
    dto.timestamp = secondsToTimePoint(1640995200);
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()