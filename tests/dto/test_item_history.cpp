#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/item_history.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ItemHistoryTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ItemHistory dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.itemId.has_value());
    BOOST_TEST(!dto.userId.has_value());
    BOOST_TEST(!dto.timestamp.has_value());
    BOOST_TEST(!dto.diff.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ItemHistory dto;

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
    // Проверка поля: itemId
    {
        BOOST_TEST(!dto.itemId.has_value());

        int64_t testValue =42;
        dto.itemId = testValue;

        BOOST_TEST(dto.itemId.has_value());

        BOOST_TEST(dto.itemId.value() == testValue);

        // Проверка сброса значения
        dto.itemId = std::nullopt;
        BOOST_TEST(!dto.itemId.has_value());
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
    // Проверка поля: diff
    {
        BOOST_TEST(!dto.diff.has_value());

        std::string testValue ="test_value";
        dto.diff = testValue;

        BOOST_TEST(dto.diff.has_value());

        BOOST_TEST(dto.diff.value() == testValue);

        // Проверка сброса значения
        dto.diff = std::nullopt;
        BOOST_TEST(!dto.diff.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ItemHistory dto;

    // Поле: id
    dto.id = 42;
    // Поле: itemId
    dto.itemId = 42;
    // Поле: userId
    dto.userId = 42;
    // Поле: timestamp
    dto.timestamp = secondsToTimePoint(1640995200);
    // Поле: diff
    dto.diff = "test_diff";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_id"));
    BOOST_TEST(json["item_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("user_id"));
    BOOST_TEST(json["user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("timestamp"));
    BOOST_TEST(json["timestamp"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("diff"));
    BOOST_TEST(json["diff"].get<std::string>() == "test_diff");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["item_id"] = 42;
    json["user_id"] = 42;
    json["timestamp"] = 1640995200;
    json["diff"] = "test_diff";

    ItemHistory dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.itemId.has_value());
    BOOST_TEST(dto.itemId.value() == 42);
    BOOST_TEST(dto.userId.has_value());
    BOOST_TEST(dto.userId.value() == 42);
    BOOST_TEST(dto.timestamp.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.timestamp.value()), 1640995200);
    BOOST_TEST(dto.diff.has_value());
    BOOST_TEST(dto.diff.value() == "test_diff");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ItemHistory original;

    // Поле: id
    original.id = 42;
    // Поле: itemId
    original.itemId = 42;
    // Поле: userId
    original.userId = 42;
    // Поле: timestamp
    original.timestamp = secondsToTimePoint(1640995200);
    // Поле: diff
    original.diff = "test_diff";

    nlohmann::json json = original.toJson();
    ItemHistory deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ItemHistory dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.itemId = 42;
    dto.userId = 42;
    dto.timestamp = secondsToTimePoint(1640995200);

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ItemHistory dto1;
    ItemHistory dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле itemId, чтобы сделать их разными
    dto1.itemId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ItemHistory dto;

    dto.itemId = 42;
    dto.userId = 42;
    dto.timestamp = secondsToTimePoint(1640995200);

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()