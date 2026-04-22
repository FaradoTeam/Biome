#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/comment.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(CommentTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Comment dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.userId.has_value());
    BOOST_TEST(!dto.itemId.has_value());
    BOOST_TEST(!dto.createdAt.has_value());
    BOOST_TEST(!dto.content.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Comment dto;

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
    // Проверка поля: createdAt
    {
        BOOST_TEST(!dto.createdAt.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.createdAt = testValue;

        BOOST_TEST(dto.createdAt.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.createdAt.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.createdAt = std::nullopt;
        BOOST_TEST(!dto.createdAt.has_value());
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
    Comment dto;

    // Поле: id
    dto.id = 42;
    // Поле: userId
    dto.userId = 42;
    // Поле: itemId
    dto.itemId = 42;
    // Поле: createdAt
    dto.createdAt = secondsToTimePoint(1640995200);
    // Поле: content
    dto.content = "test_content";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("user_id"));
    BOOST_TEST(json["user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_id"));
    BOOST_TEST(json["item_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("created_at"));
    BOOST_TEST(json["created_at"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("content"));
    BOOST_TEST(json["content"].get<std::string>() == "test_content");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["user_id"] = 42;
    json["item_id"] = 42;
    json["created_at"] = 1640995200;
    json["content"] = "test_content";

    Comment dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.userId.has_value());
    BOOST_TEST(dto.userId.value() == 42);
    BOOST_TEST(dto.itemId.has_value());
    BOOST_TEST(dto.itemId.value() == 42);
    BOOST_TEST(dto.createdAt.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.createdAt.value()), 1640995200);
    BOOST_TEST(dto.content.has_value());
    BOOST_TEST(dto.content.value() == "test_content");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Comment original;

    // Поле: id
    original.id = 42;
    // Поле: userId
    original.userId = 42;
    // Поле: itemId
    original.itemId = 42;
    // Поле: createdAt
    original.createdAt = secondsToTimePoint(1640995200);
    // Поле: content
    original.content = "test_content";

    nlohmann::json json = original.toJson();
    Comment deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Comment dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.userId = 42;
    dto.itemId = 42;
    dto.content = "test_content";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Comment dto1;
    Comment dto2;

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
    Comment dto;

    dto.userId = 42;
    dto.itemId = 42;
    dto.content = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()