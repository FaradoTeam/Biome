#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/edge.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(EdgeTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Edge dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.beginStateId.has_value());
    BOOST_TEST(!dto.endStateId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Edge dto;

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
    // Проверка поля: beginStateId
    {
        BOOST_TEST(!dto.beginStateId.has_value());

        int64_t testValue =42;
        dto.beginStateId = testValue;

        BOOST_TEST(dto.beginStateId.has_value());

        BOOST_TEST(dto.beginStateId.value() == testValue);

        // Проверка сброса значения
        dto.beginStateId = std::nullopt;
        BOOST_TEST(!dto.beginStateId.has_value());
    }
    // Проверка поля: endStateId
    {
        BOOST_TEST(!dto.endStateId.has_value());

        int64_t testValue =42;
        dto.endStateId = testValue;

        BOOST_TEST(dto.endStateId.has_value());

        BOOST_TEST(dto.endStateId.value() == testValue);

        // Проверка сброса значения
        dto.endStateId = std::nullopt;
        BOOST_TEST(!dto.endStateId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Edge dto;

    // Поле: id
    dto.id = 42;
    // Поле: beginStateId
    dto.beginStateId = 42;
    // Поле: endStateId
    dto.endStateId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("begin_state_id"));
    BOOST_TEST(json["begin_state_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("end_state_id"));
    BOOST_TEST(json["end_state_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["begin_state_id"] = 42;
    json["end_state_id"] = 42;

    Edge dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.beginStateId.has_value());
    BOOST_TEST(dto.beginStateId.value() == 42);
    BOOST_TEST(dto.endStateId.has_value());
    BOOST_TEST(dto.endStateId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Edge original;

    // Поле: id
    original.id = 42;
    // Поле: beginStateId
    original.beginStateId = 42;
    // Поле: endStateId
    original.endStateId = 42;

    nlohmann::json json = original.toJson();
    Edge deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Edge dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.beginStateId = 42;
    dto.endStateId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Edge dto1;
    Edge dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле beginStateId, чтобы сделать их разными
    dto1.beginStateId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    Edge dto;

    dto.beginStateId = 42;
    dto.endStateId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()