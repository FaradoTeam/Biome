#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/field_type.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(FieldTypeTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    FieldType dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.itemTypeId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.valueType.has_value());
    BOOST_TEST(!dto.isBoardVisible.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    FieldType dto;

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
    // Проверка поля: description
    {
        BOOST_TEST(!dto.description.has_value());

        std::string testValue ="test_value";
        dto.description = testValue;

        BOOST_TEST(dto.description.has_value());

        BOOST_TEST(dto.description.value() == testValue);

        // Проверка сброса значения
        dto.description = std::nullopt;
        BOOST_TEST(!dto.description.has_value());
    }
    // Проверка поля: valueType
    {
        BOOST_TEST(!dto.valueType.has_value());

        std::string testValue ="test_value";
        dto.valueType = testValue;

        BOOST_TEST(dto.valueType.has_value());

        BOOST_TEST(dto.valueType.value() == testValue);

        // Проверка сброса значения
        dto.valueType = std::nullopt;
        BOOST_TEST(!dto.valueType.has_value());
    }
    // Проверка поля: isBoardVisible
    {
        BOOST_TEST(!dto.isBoardVisible.has_value());

        bool testValue =true;
        dto.isBoardVisible = testValue;

        BOOST_TEST(dto.isBoardVisible.has_value());

        BOOST_TEST(dto.isBoardVisible.value() == testValue);

        // Проверка сброса значения
        dto.isBoardVisible = std::nullopt;
        BOOST_TEST(!dto.isBoardVisible.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    FieldType dto;

    // Поле: id
    dto.id = 42;
    // Поле: itemTypeId
    dto.itemTypeId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: valueType
    dto.valueType = "test_value_type";
    // Поле: isBoardVisible
    dto.isBoardVisible = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_type_id"));
    BOOST_TEST(json["item_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("value_type"));
    BOOST_TEST(json["value_type"].get<std::string>() == "test_value_type");
    BOOST_TEST(json.contains("is_board_visible"));
    BOOST_TEST(json["is_board_visible"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["item_type_id"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["value_type"] = "test_value_type";
    json["is_board_visible"] = true;

    FieldType dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.itemTypeId.has_value());
    BOOST_TEST(dto.itemTypeId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.valueType.has_value());
    BOOST_TEST(dto.valueType.value() == "test_value_type");
    BOOST_TEST(dto.isBoardVisible.has_value());
    BOOST_TEST(dto.isBoardVisible.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    FieldType original;

    // Поле: id
    original.id = 42;
    // Поле: itemTypeId
    original.itemTypeId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: valueType
    original.valueType = "test_value_type";
    // Поле: isBoardVisible
    original.isBoardVisible = true;

    nlohmann::json json = original.toJson();
    FieldType deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    FieldType dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.itemTypeId = 42;
    dto.caption = "test_caption";
    dto.valueType = "test_value_type";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    FieldType dto1;
    FieldType dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле itemTypeId, чтобы сделать их разными
    dto1.itemTypeId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    FieldType dto;

    dto.itemTypeId = 42;
    dto.caption = "test_value";
    dto.valueType = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()