#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/rule_item_type.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(RuleItemTypeTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    RuleItemType dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.ruleId.has_value());
    BOOST_TEST(!dto.itemTypeId.has_value());
    BOOST_TEST(!dto.isReader.has_value());
    BOOST_TEST(!dto.isWriter.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    RuleItemType dto;

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
    // Проверка поля: itemTypeId
    {
        BOOST_TEST(!dto.itemTypeId.has_value());

        int64_t testValue =42;
        dto.itemTypeId = testValue;

        BOOST_TEST(dto.itemTypeId.has_value());

        BOOST_TEST(dto.itemTypeId.value() == testValue);

        // Проверка сброса значения
        dto.itemTypeId = std::nullopt;
        BOOST_TEST(!dto.itemTypeId.has_value());
    }
    // Проверка поля: isReader
    {
        BOOST_TEST(!dto.isReader.has_value());

        bool testValue =true;
        dto.isReader = testValue;

        BOOST_TEST(dto.isReader.has_value());

        BOOST_TEST(dto.isReader.value() == testValue);

        // Проверка сброса значения
        dto.isReader = std::nullopt;
        BOOST_TEST(!dto.isReader.has_value());
    }
    // Проверка поля: isWriter
    {
        BOOST_TEST(!dto.isWriter.has_value());

        bool testValue =true;
        dto.isWriter = testValue;

        BOOST_TEST(dto.isWriter.has_value());

        BOOST_TEST(dto.isWriter.value() == testValue);

        // Проверка сброса значения
        dto.isWriter = std::nullopt;
        BOOST_TEST(!dto.isWriter.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    RuleItemType dto;

    // Поле: id
    dto.id = 42;
    // Поле: ruleId
    dto.ruleId = 42;
    // Поле: itemTypeId
    dto.itemTypeId = 42;
    // Поле: isReader
    dto.isReader = true;
    // Поле: isWriter
    dto.isWriter = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("rule_id"));
    BOOST_TEST(json["rule_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_type_id"));
    BOOST_TEST(json["item_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("is_reader"));
    BOOST_TEST(json["is_reader"].get<bool>() == true);
    BOOST_TEST(json.contains("is_writer"));
    BOOST_TEST(json["is_writer"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["rule_id"] = 42;
    json["item_type_id"] = 42;
    json["is_reader"] = true;
    json["is_writer"] = true;

    RuleItemType dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.ruleId.has_value());
    BOOST_TEST(dto.ruleId.value() == 42);
    BOOST_TEST(dto.itemTypeId.has_value());
    BOOST_TEST(dto.itemTypeId.value() == 42);
    BOOST_TEST(dto.isReader.has_value());
    BOOST_TEST(dto.isReader.value() == true);
    BOOST_TEST(dto.isWriter.has_value());
    BOOST_TEST(dto.isWriter.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    RuleItemType original;

    // Поле: id
    original.id = 42;
    // Поле: ruleId
    original.ruleId = 42;
    // Поле: itemTypeId
    original.itemTypeId = 42;
    // Поле: isReader
    original.isReader = true;
    // Поле: isWriter
    original.isWriter = true;

    nlohmann::json json = original.toJson();
    RuleItemType deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    RuleItemType dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.ruleId = 42;
    dto.itemTypeId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    RuleItemType dto1;
    RuleItemType dto2;

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
    RuleItemType dto;

    dto.ruleId = 42;
    dto.itemTypeId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()