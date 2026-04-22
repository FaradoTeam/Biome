#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/rule_state.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(RuleStateTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    RuleState dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.ruleId.has_value());
    BOOST_TEST(!dto.stateId.has_value());
    BOOST_TEST(!dto.isStateAllowed.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    RuleState dto;

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
    // Проверка поля: ruleId
    {
        BOOST_TEST(!dto.ruleId.has_value());

        int64_t testValue =42;
        dto.ruleId = testValue;

        BOOST_TEST(dto.ruleId.has_value());

        BOOST_TEST(dto.ruleId.value() == testValue);

        // Проверка сброса значения
        dto.ruleId = std::nullopt;
        BOOST_TEST(!dto.ruleId.has_value());
    }
    // Проверка поля: stateId
    {
        BOOST_TEST(!dto.stateId.has_value());

        int64_t testValue =42;
        dto.stateId = testValue;

        BOOST_TEST(dto.stateId.has_value());

        BOOST_TEST(dto.stateId.value() == testValue);

        // Проверка сброса значения
        dto.stateId = std::nullopt;
        BOOST_TEST(!dto.stateId.has_value());
    }
    // Проверка поля: isStateAllowed
    {
        BOOST_TEST(!dto.isStateAllowed.has_value());

        bool testValue =true;
        dto.isStateAllowed = testValue;

        BOOST_TEST(dto.isStateAllowed.has_value());

        BOOST_TEST(dto.isStateAllowed.value() == testValue);

        // Проверка сброса значения
        dto.isStateAllowed = std::nullopt;
        BOOST_TEST(!dto.isStateAllowed.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    RuleState dto;

    // Поле: id
    dto.id = 42;
    // Поле: ruleId
    dto.ruleId = 42;
    // Поле: stateId
    dto.stateId = 42;
    // Поле: isStateAllowed
    dto.isStateAllowed = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("rule_id"));
    BOOST_TEST(json["rule_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("state_id"));
    BOOST_TEST(json["state_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("is_state_allowed"));
    BOOST_TEST(json["is_state_allowed"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["rule_id"] = 42;
    json["state_id"] = 42;
    json["is_state_allowed"] = true;

    RuleState dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.ruleId.has_value());
    BOOST_TEST(dto.ruleId.value() == 42);
    BOOST_TEST(dto.stateId.has_value());
    BOOST_TEST(dto.stateId.value() == 42);
    BOOST_TEST(dto.isStateAllowed.has_value());
    BOOST_TEST(dto.isStateAllowed.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    RuleState original;

    // Поле: id
    original.id = 42;
    // Поле: ruleId
    original.ruleId = 42;
    // Поле: stateId
    original.stateId = 42;
    // Поле: isStateAllowed
    original.isStateAllowed = true;

    nlohmann::json json = original.toJson();
    RuleState deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    RuleState dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.ruleId = 42;
    dto.stateId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    RuleState dto1;
    RuleState dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле ruleId, чтобы сделать их разными
    dto1.ruleId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    RuleState dto;

    dto.ruleId = 42;
    dto.stateId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()