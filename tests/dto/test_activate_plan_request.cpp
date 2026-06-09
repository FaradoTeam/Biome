#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/activate_plan_request.h"

#include <optional>

#include "common/types.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ActivatePlanRequestTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ActivatePlanRequest dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.activatedByUserId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ActivatePlanRequest dto;

    // Проверка поля: activatedByUserId
    {
        BOOST_TEST(!dto.activatedByUserId.has_value());

        int64_t testValue =42;
        dto.activatedByUserId = testValue;

        BOOST_TEST(dto.activatedByUserId.has_value());

        BOOST_TEST(dto.activatedByUserId.value() == testValue);

        // Проверка сброса значения
        dto.activatedByUserId = std::nullopt;
        BOOST_TEST(!dto.activatedByUserId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ActivatePlanRequest dto;

    // Поле: activatedByUserId
    dto.activatedByUserId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("activatedByUserId"));
    BOOST_TEST(json["activatedByUserId"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["activatedByUserId"] = 42;

    ActivatePlanRequest dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.activatedByUserId.has_value());
    BOOST_TEST(dto.activatedByUserId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ActivatePlanRequest original;

    // Поле: activatedByUserId
    original.activatedByUserId = 42;

    nlohmann::json json = original.toJson();
    ActivatePlanRequest deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ActivatePlanRequest dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.activatedByUserId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ActivatePlanRequest dto1;
    ActivatePlanRequest dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле activatedByUserId, чтобы сделать их разными
    dto1.activatedByUserId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ActivatePlanRequest dto;

    dto.activatedByUserId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()