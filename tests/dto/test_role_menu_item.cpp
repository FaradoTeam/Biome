#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/role_menu_item.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(RoleMenuItemTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    RoleMenuItem dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.roleId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.link.has_value());
    BOOST_TEST(!dto.icon.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    RoleMenuItem dto;

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
    // Проверка поля: caption
    {
        BOOST_TEST(!dto.caption.has_value());

        std::string testValue ="test_value";
        dto.caption = testValue;

        BOOST_TEST(dto.caption.has_value());

        BOOST_TEST(dto.caption.value() == testValue);

        // Проверка сброса значения
        dto.caption = std::nullopt;
        BOOST_TEST(!dto.caption.has_value());
    }
    // Проверка поля: link
    {
        BOOST_TEST(!dto.link.has_value());

        std::string testValue ="test_value";
        dto.link = testValue;

        BOOST_TEST(dto.link.has_value());

        BOOST_TEST(dto.link.value() == testValue);

        // Проверка сброса значения
        dto.link = std::nullopt;
        BOOST_TEST(!dto.link.has_value());
    }
    // Проверка поля: icon
    {
        BOOST_TEST(!dto.icon.has_value());

        std::string testValue ="test_value";
        dto.icon = testValue;

        BOOST_TEST(dto.icon.has_value());

        BOOST_TEST(dto.icon.value() == testValue);

        // Проверка сброса значения
        dto.icon = std::nullopt;
        BOOST_TEST(!dto.icon.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    RoleMenuItem dto;

    // Поле: id
    dto.id = 42;
    // Поле: roleId
    dto.roleId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: link
    dto.link = "test_link";
    // Поле: icon
    dto.icon = "test_icon";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("role_id"));
    BOOST_TEST(json["role_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("link"));
    BOOST_TEST(json["link"].get<std::string>() == "test_link");
    BOOST_TEST(json.contains("icon"));
    BOOST_TEST(json["icon"].get<std::string>() == "test_icon");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["role_id"] = 42;
    json["caption"] = "test_caption";
    json["link"] = "test_link";
    json["icon"] = "test_icon";

    RoleMenuItem dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.roleId.has_value());
    BOOST_TEST(dto.roleId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.link.has_value());
    BOOST_TEST(dto.link.value() == "test_link");
    BOOST_TEST(dto.icon.has_value());
    BOOST_TEST(dto.icon.value() == "test_icon");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    RoleMenuItem original;

    // Поле: id
    original.id = 42;
    // Поле: roleId
    original.roleId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: link
    original.link = "test_link";
    // Поле: icon
    original.icon = "test_icon";

    nlohmann::json json = original.toJson();
    RoleMenuItem deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    RoleMenuItem dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.roleId = 42;
    dto.caption = "test_caption";
    dto.link = "test_link";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    RoleMenuItem dto1;
    RoleMenuItem dto2;

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
    RoleMenuItem dto;

    dto.roleId = 42;
    dto.caption = "test_value";
    dto.link = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()