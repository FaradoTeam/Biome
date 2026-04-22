#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/item_type.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ItemTypeTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ItemType dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.workflowId.has_value());
    BOOST_TEST(!dto.defaultStateId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.kind.has_value());
    BOOST_TEST(!dto.defaultContent.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ItemType dto;

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
    // Проверка поля: workflowId
    {
        BOOST_TEST(!dto.workflowId.has_value());

        int64_t testValue =42;
        dto.workflowId = testValue;

        BOOST_TEST(dto.workflowId.has_value());

        BOOST_TEST(dto.workflowId.value() == testValue);

        // Проверка сброса значения
        dto.workflowId = std::nullopt;
        BOOST_TEST(!dto.workflowId.has_value());
    }
    // Проверка поля: defaultStateId
    {
        BOOST_TEST(!dto.defaultStateId.has_value());

        int64_t testValue =42;
        dto.defaultStateId = testValue;

        BOOST_TEST(dto.defaultStateId.has_value());

        BOOST_TEST(dto.defaultStateId.value() == testValue);

        // Проверка сброса значения
        dto.defaultStateId = std::nullopt;
        BOOST_TEST(!dto.defaultStateId.has_value());
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
    // Проверка поля: kind
    {
        BOOST_TEST(!dto.kind.has_value());

        std::string testValue ="test_value";
        dto.kind = testValue;

        BOOST_TEST(dto.kind.has_value());

        BOOST_TEST(dto.kind.value() == testValue);

        // Проверка сброса значения
        dto.kind = std::nullopt;
        BOOST_TEST(!dto.kind.has_value());
    }
    // Проверка поля: defaultContent
    {
        BOOST_TEST(!dto.defaultContent.has_value());

        std::string testValue ="test_value";
        dto.defaultContent = testValue;

        BOOST_TEST(dto.defaultContent.has_value());

        BOOST_TEST(dto.defaultContent.value() == testValue);

        // Проверка сброса значения
        dto.defaultContent = std::nullopt;
        BOOST_TEST(!dto.defaultContent.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ItemType dto;

    // Поле: id
    dto.id = 42;
    // Поле: workflowId
    dto.workflowId = 42;
    // Поле: defaultStateId
    dto.defaultStateId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: kind
    dto.kind = "test_kind";
    // Поле: defaultContent
    dto.defaultContent = "test_default_content";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("workflow_id"));
    BOOST_TEST(json["workflow_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("default_state_id"));
    BOOST_TEST(json["default_state_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("kind"));
    BOOST_TEST(json["kind"].get<std::string>() == "test_kind");
    BOOST_TEST(json.contains("default_content"));
    BOOST_TEST(json["default_content"].get<std::string>() == "test_default_content");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["workflow_id"] = 42;
    json["default_state_id"] = 42;
    json["caption"] = "test_caption";
    json["kind"] = "test_kind";
    json["default_content"] = "test_default_content";

    ItemType dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.workflowId.has_value());
    BOOST_TEST(dto.workflowId.value() == 42);
    BOOST_TEST(dto.defaultStateId.has_value());
    BOOST_TEST(dto.defaultStateId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.kind.has_value());
    BOOST_TEST(dto.kind.value() == "test_kind");
    BOOST_TEST(dto.defaultContent.has_value());
    BOOST_TEST(dto.defaultContent.value() == "test_default_content");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ItemType original;

    // Поле: id
    original.id = 42;
    // Поле: workflowId
    original.workflowId = 42;
    // Поле: defaultStateId
    original.defaultStateId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: kind
    original.kind = "test_kind";
    // Поле: defaultContent
    original.defaultContent = "test_default_content";

    nlohmann::json json = original.toJson();
    ItemType deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ItemType dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.workflowId = 42;
    dto.defaultStateId = 42;
    dto.caption = "test_caption";
    dto.kind = "test_kind";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ItemType dto1;
    ItemType dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле workflowId, чтобы сделать их разными
    dto1.workflowId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ItemType dto;

    dto.workflowId = 42;
    dto.defaultStateId = 42;
    dto.caption = "test_value";
    dto.kind = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()