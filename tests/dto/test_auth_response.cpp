#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/auth_response.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(AuthResponseTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    AuthResponse dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.accessToken.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    AuthResponse dto;

    // Проверка поля: accessToken
    {
        BOOST_TEST(!dto.accessToken.has_value());

        std::string testValue ="test_value";
        dto.accessToken = testValue;

        BOOST_TEST(dto.accessToken.has_value());

        BOOST_TEST(dto.accessToken.value() == testValue);

        // Проверка сброса значения
        dto.accessToken = std::nullopt;
        BOOST_TEST(!dto.accessToken.has_value());
    }
    // Проверка поля: tokenType
    {
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    AuthResponse dto;

    // Поле: accessToken
    dto.accessToken = "test_access_token";
    // Поле: tokenType

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("access_token"));
    BOOST_TEST(json["access_token"].get<std::string>() == "test_access_token");
    // Константное поле tokenType должно иметь значение по умолчанию
    BOOST_TEST(json.contains("token_type"));
    BOOST_TEST(json["token_type"].get<std::string>() == "Bearer");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["access_token"] = "test_access_token";

    AuthResponse dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.accessToken.has_value());
    BOOST_TEST(dto.accessToken.value() == "test_access_token");
    // Константное поле должно иметь значение по умолчанию
    BOOST_TEST(dto.tokenType == "Bearer");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    AuthResponse original;

    // Поле: accessToken
    original.accessToken = "test_access_token";
    // Поле: tokenType

    nlohmann::json json = original.toJson();
    AuthResponse deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    AuthResponse dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());

    // Заполняем обязательные поля

    // Теперь должен быть валидным
    // Если обязательных полей нет, DTO всегда валиден
    BOOST_TEST(dto.isValid());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    AuthResponse dto1;
    AuthResponse dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));


    // Если нет обязательных полей, используем первое неконстантное поле
    dto1.accessToken = "different_value";

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));
}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    AuthResponse dto;


    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()