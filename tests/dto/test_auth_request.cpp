#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/auth_request.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(AuthRequestTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    AuthRequest dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.login.has_value());
    BOOST_TEST(!dto.password.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    AuthRequest dto;

    // Проверка поля: login
    {
        BOOST_TEST(!dto.login.has_value());

        std::string testValue ="test_value";
        dto.login = testValue;

        BOOST_TEST(dto.login.has_value());

        BOOST_TEST(dto.login.value() == testValue);

        // Проверка сброса значения
        dto.login = std::nullopt;
        BOOST_TEST(!dto.login.has_value());
    }
    // Проверка поля: password
    {
        BOOST_TEST(!dto.password.has_value());

        std::string testValue ="test_value";
        dto.password = testValue;

        BOOST_TEST(dto.password.has_value());

        BOOST_TEST(dto.password.value() == testValue);

        // Проверка сброса значения
        dto.password = std::nullopt;
        BOOST_TEST(!dto.password.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    AuthRequest dto;

    // Поле: login
    dto.login = "test_login";
    // Поле: password
    dto.password = "test_password";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("login"));
    BOOST_TEST(json["login"].get<std::string>() == "test_login");
    BOOST_TEST(json.contains("password"));
    BOOST_TEST(json["password"].get<std::string>() == "test_password");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["login"] = "test_login";
    json["password"] = "test_password";

    AuthRequest dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.login.has_value());
    BOOST_TEST(dto.login.value() == "test_login");
    BOOST_TEST(dto.password.has_value());
    BOOST_TEST(dto.password.value() == "test_password");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    AuthRequest original;

    // Поле: login
    original.login = "test_login";
    // Поле: password
    original.password = "test_password";

    nlohmann::json json = original.toJson();
    AuthRequest deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    AuthRequest dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.login = "test_login";
    dto.password = "test_password";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    AuthRequest dto1;
    AuthRequest dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле login, чтобы сделать их разными
    dto1.login = "different_value";

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    AuthRequest dto;

    dto.login = "test_value";
    dto.password = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()