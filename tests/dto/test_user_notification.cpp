#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/user_notification.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(UserNotificationTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    UserNotification dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.userId.has_value());
    BOOST_TEST(!dto.itemId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    UserNotification dto;

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
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    UserNotification dto;

    // Поле: id
    dto.id = 42;
    // Поле: userId
    dto.userId = 42;
    // Поле: itemId
    dto.itemId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("user_id"));
    BOOST_TEST(json["user_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_id"));
    BOOST_TEST(json["item_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["user_id"] = 42;
    json["item_id"] = 42;

    UserNotification dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.userId.has_value());
    BOOST_TEST(dto.userId.value() == 42);
    BOOST_TEST(dto.itemId.has_value());
    BOOST_TEST(dto.itemId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    UserNotification original;

    // Поле: id
    original.id = 42;
    // Поле: userId
    original.userId = 42;
    // Поле: itemId
    original.itemId = 42;

    nlohmann::json json = original.toJson();
    UserNotification deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    UserNotification dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.userId = 42;
    dto.itemId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    UserNotification dto1;
    UserNotification dto2;

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
    UserNotification dto;

    dto.userId = 42;
    dto.itemId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()