#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/item_link.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ItemLinkTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ItemLink dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.linkTypeId.has_value());
    BOOST_TEST(!dto.sourceItemId.has_value());
    BOOST_TEST(!dto.destinationItemId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ItemLink dto;

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
    // Проверка поля: linkTypeId
    {
        BOOST_TEST(!dto.linkTypeId.has_value());

        int64_t testValue =42;
        dto.linkTypeId = testValue;

        BOOST_TEST(dto.linkTypeId.has_value());

        BOOST_TEST(dto.linkTypeId.value() == testValue);

        // Проверка сброса значения
        dto.linkTypeId = std::nullopt;
        BOOST_TEST(!dto.linkTypeId.has_value());
    }
    // Проверка поля: sourceItemId
    {
        BOOST_TEST(!dto.sourceItemId.has_value());

        int64_t testValue =42;
        dto.sourceItemId = testValue;

        BOOST_TEST(dto.sourceItemId.has_value());

        BOOST_TEST(dto.sourceItemId.value() == testValue);

        // Проверка сброса значения
        dto.sourceItemId = std::nullopt;
        BOOST_TEST(!dto.sourceItemId.has_value());
    }
    // Проверка поля: destinationItemId
    {
        BOOST_TEST(!dto.destinationItemId.has_value());

        int64_t testValue =42;
        dto.destinationItemId = testValue;

        BOOST_TEST(dto.destinationItemId.has_value());

        BOOST_TEST(dto.destinationItemId.value() == testValue);

        // Проверка сброса значения
        dto.destinationItemId = std::nullopt;
        BOOST_TEST(!dto.destinationItemId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ItemLink dto;

    // Поле: id
    dto.id = 42;
    // Поле: linkTypeId
    dto.linkTypeId = 42;
    // Поле: sourceItemId
    dto.sourceItemId = 42;
    // Поле: destinationItemId
    dto.destinationItemId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("link_type_id"));
    BOOST_TEST(json["link_type_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("source_item_id"));
    BOOST_TEST(json["source_item_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("destination_item_id"));
    BOOST_TEST(json["destination_item_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["link_type_id"] = 42;
    json["source_item_id"] = 42;
    json["destination_item_id"] = 42;

    ItemLink dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.linkTypeId.has_value());
    BOOST_TEST(dto.linkTypeId.value() == 42);
    BOOST_TEST(dto.sourceItemId.has_value());
    BOOST_TEST(dto.sourceItemId.value() == 42);
    BOOST_TEST(dto.destinationItemId.has_value());
    BOOST_TEST(dto.destinationItemId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ItemLink original;

    // Поле: id
    original.id = 42;
    // Поле: linkTypeId
    original.linkTypeId = 42;
    // Поле: sourceItemId
    original.sourceItemId = 42;
    // Поле: destinationItemId
    original.destinationItemId = 42;

    nlohmann::json json = original.toJson();
    ItemLink deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ItemLink dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.linkTypeId = 42;
    dto.sourceItemId = 42;
    dto.destinationItemId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ItemLink dto1;
    ItemLink dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле linkTypeId, чтобы сделать их разными
    dto1.linkTypeId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ItemLink dto;

    dto.linkTypeId = 42;
    dto.sourceItemId = 42;
    dto.destinationItemId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()