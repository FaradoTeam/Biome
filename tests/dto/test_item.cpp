#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/item.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ItemTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Item dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.itemTypeId.has_value());
    BOOST_TEST(!dto.parentId.has_value());
    BOOST_TEST(!dto.stateId.has_value());
    BOOST_TEST(!dto.phaseId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.content.has_value());
    BOOST_TEST(!dto.isDeleted.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Item dto;

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
    // Проверка поля: parentId
    {
        BOOST_TEST(!dto.parentId.has_value());

        int64_t testValue =42;
        dto.parentId = testValue;

        BOOST_TEST(dto.parentId.has_value());

        BOOST_TEST(dto.parentId.value() == testValue);

        // Проверка сброса значения
        dto.parentId = std::nullopt;
        BOOST_TEST(!dto.parentId.has_value());
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
    // Проверка поля: phaseId
    {
        BOOST_TEST(!dto.phaseId.has_value());

        int64_t testValue =42;
        dto.phaseId = testValue;

        BOOST_TEST(dto.phaseId.has_value());

        BOOST_TEST(dto.phaseId.value() == testValue);

        // Проверка сброса значения
        dto.phaseId = std::nullopt;
        BOOST_TEST(!dto.phaseId.has_value());
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
    // Проверка поля: content
    {
        BOOST_TEST(!dto.content.has_value());

        std::string testValue ="test_value";
        dto.content = testValue;

        BOOST_TEST(dto.content.has_value());

        BOOST_TEST(dto.content.value() == testValue);

        // Проверка сброса значения
        dto.content = std::nullopt;
        BOOST_TEST(!dto.content.has_value());
    }
    // Проверка поля: isDeleted
    {
        BOOST_TEST(!dto.isDeleted.has_value());

        bool testValue =true;
        dto.isDeleted = testValue;

        BOOST_TEST(dto.isDeleted.has_value());

        BOOST_TEST(dto.isDeleted.value() == testValue);

        // Проверка сброса значения
        dto.isDeleted = std::nullopt;
        BOOST_TEST(!dto.isDeleted.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Item dto;

    // Поле: id
    dto.id = 42;
    // Поле: itemTypeId
    dto.itemTypeId = 42;
    // Поле: parentId
    dto.parentId = 42;
    // Поле: stateId
    dto.stateId = 42;
    // Поле: phaseId
    dto.phaseId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: content
    dto.content = "test_content";
    // Поле: isDeleted
    dto.isDeleted = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_type_id"));
    BOOST_TEST(json["item_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("parent_id"));
    BOOST_TEST(json["parent_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("state_id"));
    BOOST_TEST(json["state_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("phase_id"));
    BOOST_TEST(json["phase_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("content"));
    BOOST_TEST(json["content"].get<std::string>() == "test_content");
    BOOST_TEST(json.contains("is_deleted"));
    BOOST_TEST(json["is_deleted"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["item_type_id"] = 42;
    json["parent_id"] = 42;
    json["state_id"] = 42;
    json["phase_id"] = 42;
    json["caption"] = "test_caption";
    json["content"] = "test_content";
    json["is_deleted"] = true;

    Item dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.itemTypeId.has_value());
    BOOST_TEST(dto.itemTypeId.value() == 42);
    BOOST_TEST(dto.parentId.has_value());
    BOOST_TEST(dto.parentId.value() == 42);
    BOOST_TEST(dto.stateId.has_value());
    BOOST_TEST(dto.stateId.value() == 42);
    BOOST_TEST(dto.phaseId.has_value());
    BOOST_TEST(dto.phaseId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.content.has_value());
    BOOST_TEST(dto.content.value() == "test_content");
    BOOST_TEST(dto.isDeleted.has_value());
    BOOST_TEST(dto.isDeleted.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Item original;

    // Поле: id
    original.id = 42;
    // Поле: itemTypeId
    original.itemTypeId = 42;
    // Поле: parentId
    original.parentId = 42;
    // Поле: stateId
    original.stateId = 42;
    // Поле: phaseId
    original.phaseId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: content
    original.content = "test_content";
    // Поле: isDeleted
    original.isDeleted = true;

    nlohmann::json json = original.toJson();
    Item deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Item dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.itemTypeId = 42;
    dto.stateId = 42;
    dto.phaseId = 42;
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Item dto1;
    Item dto2;

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
    Item dto;

    dto.itemTypeId = 42;
    dto.stateId = 42;
    dto.phaseId = 42;
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()