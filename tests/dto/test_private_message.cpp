#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/private_message.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(PrivateMessageTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    PrivateMessage dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.senderUserId.has_value());
    BOOST_TEST(!dto.receiverUserId.has_value());
    BOOST_TEST(!dto.creationTimestamp.has_value());
    BOOST_TEST(!dto.content.has_value());
    BOOST_TEST(!dto.isViewed.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    PrivateMessage dto;

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
    // Проверка поля: receiverUserId
    {
        BOOST_TEST(!dto.receiverUserId.has_value());

        int64_t testValue =42;
        dto.receiverUserId = testValue;

        BOOST_TEST(dto.receiverUserId.has_value());

        BOOST_TEST(dto.receiverUserId.value() == testValue);

        // Проверка сброса значения
        dto.receiverUserId = std::nullopt;
        BOOST_TEST(!dto.receiverUserId.has_value());
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
    // Проверка поля: isViewed
    {
        BOOST_TEST(!dto.isViewed.has_value());

        bool testValue =true;
        dto.isViewed = testValue;

        BOOST_TEST(dto.isViewed.has_value());

        BOOST_TEST(dto.isViewed.value() == testValue);

        // Проверка сброса значения
        dto.isViewed = std::nullopt;
        BOOST_TEST(!dto.isViewed.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    PrivateMessage dto;

    // Поле: id
    dto.id = 42;
    // Поле: senderUserId
    dto.senderUserId = 42;
    // Поле: receiverUserId
    dto.receiverUserId = 42;
    // Поле: creationTimestamp
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    // Поле: content
    dto.content = "test_content";
    // Поле: isViewed
    dto.isViewed = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("sender_user_id"));
    BOOST_TEST(json["sender_user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("receiver_user_id"));
    BOOST_TEST(json["receiver_user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("creation_timestamp"));
    BOOST_TEST(json["creation_timestamp"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("content"));
    BOOST_TEST(json["content"].get<std::string>() == "test_content");
    BOOST_TEST(json.contains("is_viewed"));
    BOOST_TEST(json["is_viewed"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["sender_user_id"] = 42;
    json["receiver_user_id"] = 42;
    json["creation_timestamp"] = 1640995200;
    json["content"] = "test_content";
    json["is_viewed"] = true;

    PrivateMessage dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.senderUserId.has_value());
    BOOST_TEST(dto.senderUserId.value() == 42);
    BOOST_TEST(dto.receiverUserId.has_value());
    BOOST_TEST(dto.receiverUserId.value() == 42);
    BOOST_TEST(dto.creationTimestamp.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.creationTimestamp.value()), 1640995200);
    BOOST_TEST(dto.content.has_value());
    BOOST_TEST(dto.content.value() == "test_content");
    BOOST_TEST(dto.isViewed.has_value());
    BOOST_TEST(dto.isViewed.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    PrivateMessage original;

    // Поле: id
    original.id = 42;
    // Поле: senderUserId
    original.senderUserId = 42;
    // Поле: receiverUserId
    original.receiverUserId = 42;
    // Поле: creationTimestamp
    original.creationTimestamp = secondsToTimePoint(1640995200);
    // Поле: content
    original.content = "test_content";
    // Поле: isViewed
    original.isViewed = true;

    nlohmann::json json = original.toJson();
    PrivateMessage deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    PrivateMessage dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.senderUserId = 42;
    dto.receiverUserId = 42;
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    dto.content = "test_content";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    PrivateMessage dto1;
    PrivateMessage dto2;

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
    PrivateMessage dto;

    dto.senderUserId = 42;
    dto.receiverUserId = 42;
    dto.creationTimestamp = secondsToTimePoint(1640995200);
    dto.content = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()