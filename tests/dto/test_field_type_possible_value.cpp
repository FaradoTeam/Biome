#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/field_type_possible_value.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(FieldTypePossibleValueTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    FieldTypePossibleValue dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.fieldTypeId.has_value());
    BOOST_TEST(!dto.value.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    FieldTypePossibleValue dto;

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
    // Проверка поля: fieldTypeId
    {
        BOOST_TEST(!dto.fieldTypeId.has_value());

        int64_t testValue =42;
        dto.fieldTypeId = testValue;

        BOOST_TEST(dto.fieldTypeId.has_value());

        BOOST_TEST(dto.fieldTypeId.value() == testValue);

        // Проверка сброса значения
        dto.fieldTypeId = std::nullopt;
        BOOST_TEST(!dto.fieldTypeId.has_value());
    }
    // Проверка поля: value
    {
        BOOST_TEST(!dto.value.has_value());

        std::string testValue ="test_value";
        dto.value = testValue;

        BOOST_TEST(dto.value.has_value());

        BOOST_TEST(dto.value.value() == testValue);

        // Проверка сброса значения
        dto.value = std::nullopt;
        BOOST_TEST(!dto.value.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    FieldTypePossibleValue dto;

    // Поле: id
    dto.id = 42;
    // Поле: fieldTypeId
    dto.fieldTypeId = 42;
    // Поле: value
    dto.value = "test_value";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("field_type_id"));
    BOOST_TEST(json["field_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("value"));
    BOOST_TEST(json["value"].get<std::string>() == "test_value");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["field_type_id"] = 42;
    json["value"] = "test_value";

    FieldTypePossibleValue dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.fieldTypeId.has_value());
    BOOST_TEST(dto.fieldTypeId.value() == 42);
    BOOST_TEST(dto.value.has_value());
    BOOST_TEST(dto.value.value() == "test_value");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    FieldTypePossibleValue original;

    // Поле: id
    original.id = 42;
    // Поле: fieldTypeId
    original.fieldTypeId = 42;
    // Поле: value
    original.value = "test_value";

    nlohmann::json json = original.toJson();
    FieldTypePossibleValue deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    FieldTypePossibleValue dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.fieldTypeId = 42;
    dto.value = "test_value";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    FieldTypePossibleValue dto1;
    FieldTypePossibleValue dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле fieldTypeId, чтобы сделать их разными
    dto1.fieldTypeId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    FieldTypePossibleValue dto;

    dto.fieldTypeId = 42;
    dto.value = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()