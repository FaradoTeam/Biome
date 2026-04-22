#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/link_type.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(LinkTypeTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    LinkType dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.sourceItemTypeId.has_value());
    BOOST_TEST(!dto.destinationItemTypeId.has_value());
    BOOST_TEST(!dto.isBidirectional.has_value());
    BOOST_TEST(!dto.caption.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    LinkType dto;

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
    // Проверка поля: sourceItemTypeId
    {
        BOOST_TEST(!dto.sourceItemTypeId.has_value());

        int64_t testValue =42;
        dto.sourceItemTypeId = testValue;

        BOOST_TEST(dto.sourceItemTypeId.has_value());

        BOOST_TEST(dto.sourceItemTypeId.value() == testValue);

        // Проверка сброса значения
        dto.sourceItemTypeId = std::nullopt;
        BOOST_TEST(!dto.sourceItemTypeId.has_value());
    }
    // Проверка поля: destinationItemTypeId
    {
        BOOST_TEST(!dto.destinationItemTypeId.has_value());

        int64_t testValue =42;
        dto.destinationItemTypeId = testValue;

        BOOST_TEST(dto.destinationItemTypeId.has_value());

        BOOST_TEST(dto.destinationItemTypeId.value() == testValue);

        // Проверка сброса значения
        dto.destinationItemTypeId = std::nullopt;
        BOOST_TEST(!dto.destinationItemTypeId.has_value());
    }
    // Проверка поля: isBidirectional
    {
        BOOST_TEST(!dto.isBidirectional.has_value());

        bool testValue =true;
        dto.isBidirectional = testValue;

        BOOST_TEST(dto.isBidirectional.has_value());

        BOOST_TEST(dto.isBidirectional.value() == testValue);

        // Проверка сброса значения
        dto.isBidirectional = std::nullopt;
        BOOST_TEST(!dto.isBidirectional.has_value());
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
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    LinkType dto;

    // Поле: id
    dto.id = 42;
    // Поле: sourceItemTypeId
    dto.sourceItemTypeId = 42;
    // Поле: destinationItemTypeId
    dto.destinationItemTypeId = 42;
    // Поле: isBidirectional
    dto.isBidirectional = true;
    // Поле: caption
    dto.caption = "test_caption";

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("source_item_type_id"));
    BOOST_TEST(json["source_item_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("destination_item_type_id"));
    BOOST_TEST(json["destination_item_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("is_bidirectional"));
    BOOST_TEST(json["is_bidirectional"].get<bool>() == true);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["source_item_type_id"] = 42;
    json["destination_item_type_id"] = 42;
    json["is_bidirectional"] = true;
    json["caption"] = "test_caption";

    LinkType dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.sourceItemTypeId.has_value());
    BOOST_TEST(dto.sourceItemTypeId.value() == 42);
    BOOST_TEST(dto.destinationItemTypeId.has_value());
    BOOST_TEST(dto.destinationItemTypeId.value() == 42);
    BOOST_TEST(dto.isBidirectional.has_value());
    BOOST_TEST(dto.isBidirectional.value() == true);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    LinkType original;

    // Поле: id
    original.id = 42;
    // Поле: sourceItemTypeId
    original.sourceItemTypeId = 42;
    // Поле: destinationItemTypeId
    original.destinationItemTypeId = 42;
    // Поле: isBidirectional
    original.isBidirectional = true;
    // Поле: caption
    original.caption = "test_caption";

    nlohmann::json json = original.toJson();
    LinkType deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    LinkType dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.sourceItemTypeId = 42;
    dto.destinationItemTypeId = 42;
    dto.isBidirectional = true;
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    LinkType dto1;
    LinkType dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле sourceItemTypeId, чтобы сделать их разными
    dto1.sourceItemTypeId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    LinkType dto;

    dto.sourceItemTypeId = 42;
    dto.destinationItemTypeId = 42;
    dto.isBidirectional = true;
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()