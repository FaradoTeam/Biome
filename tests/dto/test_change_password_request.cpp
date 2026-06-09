#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/change_password_request.h"

#include <optional>

#include "common/types.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ChangePasswordRequestTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ChangePasswordRequest dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.oldPassword.has_value());
    BOOST_TEST(!dto.newPassword.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ChangePasswordRequest dto;

    // Проверка поля: oldPassword
    {
        BOOST_TEST(!dto.oldPassword.has_value());

        std::string testValue ="test_value";
        dto.oldPassword = testValue;

        BOOST_TEST(dto.oldPassword.has_value());

        BOOST_TEST(dto.oldPassword.value() == testValue);

        // Проверка сброса значения
        dto.oldPassword = std::nullopt;
        BOOST_TEST(!dto.oldPassword.has_value());
    }
    // Проверка поля: newPassword
    {
        BOOST_TEST(!dto.newPassword.has_value());

        std::string testValue ="test_value";
        dto.newPassword = testValue;

        BOOST_TEST(dto.newPassword.has_value());

        BOOST_TEST(dto.newPassword.value() == testValue);

        // Проверка сброса значения
        dto.newPassword = std::nullopt;
        BOOST_TEST(!dto.newPassword.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ChangePasswordRequest dto;

    // Поле: oldPassword
    dto.oldPassword = "test_oldPassword";
    // Поле: newPassword
    dto.newPassword = "test_newPassword";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("oldPassword"));
    BOOST_TEST(json["oldPassword"].get<std::string>() == "test_oldPassword");
    BOOST_TEST(json.contains("newPassword"));
    BOOST_TEST(json["newPassword"].get<std::string>() == "test_newPassword");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["oldPassword"] = "test_oldPassword";
    json["newPassword"] = "test_newPassword";

    ChangePasswordRequest dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.oldPassword.has_value());
    BOOST_TEST(dto.oldPassword.value() == "test_oldPassword");
    BOOST_TEST(dto.newPassword.has_value());
    BOOST_TEST(dto.newPassword.value() == "test_newPassword");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ChangePasswordRequest original;

    // Поле: oldPassword
    original.oldPassword = "test_oldPassword";
    // Поле: newPassword
    original.newPassword = "test_newPassword";

    nlohmann::json json = original.toJson();
    ChangePasswordRequest deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ChangePasswordRequest dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.oldPassword = "test_oldPassword";
    dto.newPassword = "test_newPassword";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ChangePasswordRequest dto1;
    ChangePasswordRequest dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле oldPassword, чтобы сделать их разными
    dto1.oldPassword = "different_value";

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ChangePasswordRequest dto;

    dto.oldPassword = "test_value";
    dto.newPassword = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()