#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/error_response.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ErrorResponseTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ErrorResponse dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.code.has_value());
    BOOST_TEST(!dto.message.has_value());
    BOOST_TEST(!dto.details.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ErrorResponse dto;

    // Проверка поля: code
    {
        BOOST_TEST(!dto.code.has_value());

        int64_t testValue =42;
        dto.code = testValue;

        BOOST_TEST(dto.code.has_value());

        BOOST_TEST(dto.code.value() == testValue);

        // Проверка сброса значения
        dto.code = std::nullopt;
        BOOST_TEST(!dto.code.has_value());
    }
    // Проверка поля: message
    {
        BOOST_TEST(!dto.message.has_value());

        std::string testValue ="test_value";
        dto.message = testValue;

        BOOST_TEST(dto.message.has_value());

        BOOST_TEST(dto.message.value() == testValue);

        // Проверка сброса значения
        dto.message = std::nullopt;
        BOOST_TEST(!dto.message.has_value());
    }
    // Проверка поля: details
    {
        BOOST_TEST(!dto.details.has_value());

        std::string testValue ="test_value";
        dto.details = testValue;

        BOOST_TEST(dto.details.has_value());

        BOOST_TEST(dto.details.value() == testValue);

        // Проверка сброса значения
        dto.details = std::nullopt;
        BOOST_TEST(!dto.details.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ErrorResponse dto;

    // Поле: code
    dto.code = 42;
    // Поле: message
    dto.message = "test_message";
    // Поле: details
    dto.details = "test_details";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("code"));
    BOOST_TEST(json["code"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("message"));
    BOOST_TEST(json["message"].get<std::string>() == "test_message");
    BOOST_TEST(json.contains("details"));
    BOOST_TEST(json["details"].get<std::string>() == "test_details");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["code"] = 42;
    json["message"] = "test_message";
    json["details"] = "test_details";

    ErrorResponse dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.code.has_value());
    BOOST_TEST(dto.code.value() == 42);
    BOOST_TEST(dto.message.has_value());
    BOOST_TEST(dto.message.value() == "test_message");
    BOOST_TEST(dto.details.has_value());
    BOOST_TEST(dto.details.value() == "test_details");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ErrorResponse original;

    // Поле: code
    original.code = 42;
    // Поле: message
    original.message = "test_message";
    // Поле: details
    original.details = "test_details";

    nlohmann::json json = original.toJson();
    ErrorResponse deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ErrorResponse dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.code = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ErrorResponse dto1;
    ErrorResponse dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле code, чтобы сделать их разными
    dto1.code = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ErrorResponse dto;

    dto.code = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()