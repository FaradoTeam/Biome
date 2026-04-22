#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/user.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(UserTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    User dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.login.has_value());
    BOOST_TEST(!dto.firstName.has_value());
    BOOST_TEST(!dto.middleName.has_value());
    BOOST_TEST(!dto.lastName.has_value());
    BOOST_TEST(!dto.email.has_value());
    BOOST_TEST(!dto.needChangePassword.has_value());
    BOOST_TEST(!dto.isBlocked.has_value());
    BOOST_TEST(!dto.isSuperAdmin.has_value());
    BOOST_TEST(!dto.isHidden.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    User dto;

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
    // Проверка поля: firstName
    {
        BOOST_TEST(!dto.firstName.has_value());

        std::string testValue ="test_value";
        dto.firstName = testValue;

        BOOST_TEST(dto.firstName.has_value());

        BOOST_TEST(dto.firstName.value() == testValue);

        // Проверка сброса значения
        dto.firstName = std::nullopt;
        BOOST_TEST(!dto.firstName.has_value());
    }
    // Проверка поля: middleName
    {
        BOOST_TEST(!dto.middleName.has_value());

        std::string testValue ="test_value";
        dto.middleName = testValue;

        BOOST_TEST(dto.middleName.has_value());

        BOOST_TEST(dto.middleName.value() == testValue);

        // Проверка сброса значения
        dto.middleName = std::nullopt;
        BOOST_TEST(!dto.middleName.has_value());
    }
    // Проверка поля: lastName
    {
        BOOST_TEST(!dto.lastName.has_value());

        std::string testValue ="test_value";
        dto.lastName = testValue;

        BOOST_TEST(dto.lastName.has_value());

        BOOST_TEST(dto.lastName.value() == testValue);

        // Проверка сброса значения
        dto.lastName = std::nullopt;
        BOOST_TEST(!dto.lastName.has_value());
    }
    // Проверка поля: email
    {
        BOOST_TEST(!dto.email.has_value());

        std::string testValue ="test_value";
        dto.email = testValue;

        BOOST_TEST(dto.email.has_value());

        BOOST_TEST(dto.email.value() == testValue);

        // Проверка сброса значения
        dto.email = std::nullopt;
        BOOST_TEST(!dto.email.has_value());
    }
    // Проверка поля: needChangePassword
    {
        BOOST_TEST(!dto.needChangePassword.has_value());

        bool testValue =true;
        dto.needChangePassword = testValue;

        BOOST_TEST(dto.needChangePassword.has_value());

        BOOST_TEST(dto.needChangePassword.value() == testValue);

        // Проверка сброса значения
        dto.needChangePassword = std::nullopt;
        BOOST_TEST(!dto.needChangePassword.has_value());
    }
    // Проверка поля: isBlocked
    {
        BOOST_TEST(!dto.isBlocked.has_value());

        bool testValue =true;
        dto.isBlocked = testValue;

        BOOST_TEST(dto.isBlocked.has_value());

        BOOST_TEST(dto.isBlocked.value() == testValue);

        // Проверка сброса значения
        dto.isBlocked = std::nullopt;
        BOOST_TEST(!dto.isBlocked.has_value());
    }
    // Проверка поля: isSuperAdmin
    {
        BOOST_TEST(!dto.isSuperAdmin.has_value());

        bool testValue =true;
        dto.isSuperAdmin = testValue;

        BOOST_TEST(dto.isSuperAdmin.has_value());

        BOOST_TEST(dto.isSuperAdmin.value() == testValue);

        // Проверка сброса значения
        dto.isSuperAdmin = std::nullopt;
        BOOST_TEST(!dto.isSuperAdmin.has_value());
    }
    // Проверка поля: isHidden
    {
        BOOST_TEST(!dto.isHidden.has_value());

        bool testValue =true;
        dto.isHidden = testValue;

        BOOST_TEST(dto.isHidden.has_value());

        BOOST_TEST(dto.isHidden.value() == testValue);

        // Проверка сброса значения
        dto.isHidden = std::nullopt;
        BOOST_TEST(!dto.isHidden.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    User dto;

    // Поле: id
    dto.id = 42;
    // Поле: login
    dto.login = "test_login";
    // Поле: firstName
    dto.firstName = "test_first_name";
    // Поле: middleName
    dto.middleName = "test_middle_name";
    // Поле: lastName
    dto.lastName = "test_last_name";
    // Поле: email
    dto.email = "test_email";
    // Поле: needChangePassword
    dto.needChangePassword = true;
    // Поле: isBlocked
    dto.isBlocked = true;
    // Поле: isSuperAdmin
    dto.isSuperAdmin = true;
    // Поле: isHidden
    dto.isHidden = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("login"));
    BOOST_TEST(json["login"].get<std::string>() == "test_login");
    BOOST_TEST(json.contains("first_name"));
    BOOST_TEST(json["first_name"].get<std::string>() == "test_first_name");
    BOOST_TEST(json.contains("middle_name"));
    BOOST_TEST(json["middle_name"].get<std::string>() == "test_middle_name");
    BOOST_TEST(json.contains("last_name"));
    BOOST_TEST(json["last_name"].get<std::string>() == "test_last_name");
    BOOST_TEST(json.contains("email"));
    BOOST_TEST(json["email"].get<std::string>() == "test_email");
    BOOST_TEST(json.contains("need_change_password"));
    BOOST_TEST(json["need_change_password"].get<bool>() == true);
    BOOST_TEST(json.contains("is_blocked"));
    BOOST_TEST(json["is_blocked"].get<bool>() == true);
    BOOST_TEST(json.contains("is_super_admin"));
    BOOST_TEST(json["is_super_admin"].get<bool>() == true);
    BOOST_TEST(json.contains("is_hidden"));
    BOOST_TEST(json["is_hidden"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["login"] = "test_login";
    json["first_name"] = "test_first_name";
    json["middle_name"] = "test_middle_name";
    json["last_name"] = "test_last_name";
    json["email"] = "test_email";
    json["need_change_password"] = true;
    json["is_blocked"] = true;
    json["is_super_admin"] = true;
    json["is_hidden"] = true;

    User dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.login.has_value());
    BOOST_TEST(dto.login.value() == "test_login");
    BOOST_TEST(dto.firstName.has_value());
    BOOST_TEST(dto.firstName.value() == "test_first_name");
    BOOST_TEST(dto.middleName.has_value());
    BOOST_TEST(dto.middleName.value() == "test_middle_name");
    BOOST_TEST(dto.lastName.has_value());
    BOOST_TEST(dto.lastName.value() == "test_last_name");
    BOOST_TEST(dto.email.has_value());
    BOOST_TEST(dto.email.value() == "test_email");
    BOOST_TEST(dto.needChangePassword.has_value());
    BOOST_TEST(dto.needChangePassword.value() == true);
    BOOST_TEST(dto.isBlocked.has_value());
    BOOST_TEST(dto.isBlocked.value() == true);
    BOOST_TEST(dto.isSuperAdmin.has_value());
    BOOST_TEST(dto.isSuperAdmin.value() == true);
    BOOST_TEST(dto.isHidden.has_value());
    BOOST_TEST(dto.isHidden.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    User original;

    // Поле: id
    original.id = 42;
    // Поле: login
    original.login = "test_login";
    // Поле: firstName
    original.firstName = "test_first_name";
    // Поле: middleName
    original.middleName = "test_middle_name";
    // Поле: lastName
    original.lastName = "test_last_name";
    // Поле: email
    original.email = "test_email";
    // Поле: needChangePassword
    original.needChangePassword = true;
    // Поле: isBlocked
    original.isBlocked = true;
    // Поле: isSuperAdmin
    original.isSuperAdmin = true;
    // Поле: isHidden
    original.isHidden = true;

    nlohmann::json json = original.toJson();
    User deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    User dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.login = "test_login";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    User dto1;
    User dto2;

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
    User dto;

    dto.login = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()