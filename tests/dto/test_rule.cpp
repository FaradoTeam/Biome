#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/rule.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(RuleTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Rule dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.roleId.has_value());
    BOOST_TEST(!dto.isRootProjectCreator.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Rule dto;

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
    // Проверка поля: roleId
    {
        BOOST_TEST(!dto.roleId.has_value());

        int64_t testValue =42;
        dto.roleId = testValue;

        BOOST_TEST(dto.roleId.has_value());

        BOOST_TEST(dto.roleId.value() == testValue);

        // Проверка сброса значения
        dto.roleId = std::nullopt;
        BOOST_TEST(!dto.roleId.has_value());
    }
    // Проверка поля: isRootProjectCreator
    {
        BOOST_TEST(!dto.isRootProjectCreator.has_value());

        bool testValue =true;
        dto.isRootProjectCreator = testValue;

        BOOST_TEST(dto.isRootProjectCreator.has_value());

        BOOST_TEST(dto.isRootProjectCreator.value() == testValue);

        // Проверка сброса значения
        dto.isRootProjectCreator = std::nullopt;
        BOOST_TEST(!dto.isRootProjectCreator.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Rule dto;

    // Поле: id
    dto.id = 42;
    // Поле: roleId
    dto.roleId = 42;
    // Поле: isRootProjectCreator
    dto.isRootProjectCreator = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("role_id"));
    BOOST_TEST(json["role_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("is_root_project_creator"));
    BOOST_TEST(json["is_root_project_creator"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["role_id"] = 42;
    json["is_root_project_creator"] = true;

    Rule dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.roleId.has_value());
    BOOST_TEST(dto.roleId.value() == 42);
    BOOST_TEST(dto.isRootProjectCreator.has_value());
    BOOST_TEST(dto.isRootProjectCreator.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Rule original;

    // Поле: id
    original.id = 42;
    // Поле: roleId
    original.roleId = 42;
    // Поле: isRootProjectCreator
    original.isRootProjectCreator = true;

    nlohmann::json json = original.toJson();
    Rule deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Rule dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.roleId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Rule dto1;
    Rule dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле roleId, чтобы сделать их разными
    dto1.roleId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    Rule dto;

    dto.roleId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()