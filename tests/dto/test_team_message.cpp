#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/team_message.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(TeamMessageTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    TeamMessage dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.senderUserId.has_value());
    BOOST_TEST(!dto.teamId.has_value());
    BOOST_TEST(!dto.creationTimestamp.has_value());
    BOOST_TEST(!dto.content.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    TeamMessage dto;

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
    // Проверка поля: senderUserId
    {
        BOOST_TEST(!dto.senderUserId.has_value());

        int64_t testValue =42;
        dto.senderUserId = testValue;

        BOOST_TEST(dto.senderUserId.has_value());

        BOOST_TEST(dto.senderUserId.value() == testValue);

        // Проверка сброса значения
        dto.senderUserId = std::nullopt;
        BOOST_TEST(!dto.senderUserId.has_value());
    }
    // Проверка поля: teamId
    {
        BOOST_TEST(!dto.teamId.has_value());

        int64_t testValue =42;
        dto.teamId = testValue;

        BOOST_TEST(dto.teamId.has_value());

        BOOST_TEST(dto.teamId.value() == testValue);

        // Проверка сброса значения
        dto.teamId = std::nullopt;
        BOOST_TEST(!dto.teamId.has_value());
    }
    // Проверка поля: creationTimestamp
    {
        BOOST_TEST(!dto.creationTimestamp.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.creationTimestamp = testValue;

        BOOST_TEST(dto.creationTimestamp.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.creationTimestamp.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.creationTimestamp = std::nullopt;
        BOOST_TEST(!dto.creationTimestamp.has_value());
    }
    // Проверка поля: content
    {
        BOOST_TEST(!dto.content.has_value());

        std::string testValue ="test_value";
        dto.content = testValue;

        BOOST_TEST(dto.content.has_value());

        BOOST_TEST(dto.content.value() == testValue);

        // Проверка сброса значения
        dto.content = std::nullopt;
        BOOST_TEST(!dto.content.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    TeamMessage dto;

    // Поле: id
    dto.id = 42;
    // Поле: senderUserId
    dto.senderUserId = 42;
    // Поле: teamId
    dto.teamId = 42;
    // Поле: creationTimestamp
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    // Поле: content
    dto.content = "test_content";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("sender_user_id"));
    BOOST_TEST(json["sender_user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("team_id"));
    BOOST_TEST(json["team_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("creation_timestamp"));
    BOOST_TEST(json["creation_timestamp"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("content"));
    BOOST_TEST(json["content"].get<std::string>() == "test_content");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["sender_user_id"] = 42;
    json["team_id"] = 42;
    json["creation_timestamp"] = 1640995200;
    json["content"] = "test_content";

    TeamMessage dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.senderUserId.has_value());
    BOOST_TEST(dto.senderUserId.value() == 42);
    BOOST_TEST(dto.teamId.has_value());
    BOOST_TEST(dto.teamId.value() == 42);
    BOOST_TEST(dto.creationTimestamp.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.creationTimestamp.value()), 1640995200);
    BOOST_TEST(dto.content.has_value());
    BOOST_TEST(dto.content.value() == "test_content");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    TeamMessage original;

    // Поле: id
    original.id = 42;
    // Поле: senderUserId
    original.senderUserId = 42;
    // Поле: teamId
    original.teamId = 42;
    // Поле: creationTimestamp
    original.creationTimestamp = secondsToTimePoint(1640995200);
    // Поле: content
    original.content = "test_content";

    nlohmann::json json = original.toJson();
    TeamMessage deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    TeamMessage dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.senderUserId = 42;
    dto.teamId = 42;
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    dto.content = "test_content";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    TeamMessage dto1;
    TeamMessage dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле senderUserId, чтобы сделать их разными
    dto1.senderUserId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    TeamMessage dto;

    dto.senderUserId = 42;
    dto.teamId = 42;
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    dto.content = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()