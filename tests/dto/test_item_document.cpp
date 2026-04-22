#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/item_document.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ItemDocumentTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ItemDocument dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.itemId.has_value());
    BOOST_TEST(!dto.documentId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ItemDocument dto;

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
    // Проверка поля: itemId
    {
        BOOST_TEST(!dto.itemId.has_value());

        int64_t testValue =42;
        dto.itemId = testValue;

        BOOST_TEST(dto.itemId.has_value());

        BOOST_TEST(dto.itemId.value() == testValue);

        // Проверка сброса значения
        dto.itemId = std::nullopt;
        BOOST_TEST(!dto.itemId.has_value());
    }
    // Проверка поля: documentId
    {
        BOOST_TEST(!dto.documentId.has_value());

        int64_t testValue =42;
        dto.documentId = testValue;

        BOOST_TEST(dto.documentId.has_value());

        BOOST_TEST(dto.documentId.value() == testValue);

        // Проверка сброса значения
        dto.documentId = std::nullopt;
        BOOST_TEST(!dto.documentId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ItemDocument dto;

    // Поле: id
    dto.id = 42;
    // Поле: itemId
    dto.itemId = 42;
    // Поле: documentId
    dto.documentId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("item_id"));
    BOOST_TEST(json["item_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("document_id"));
    BOOST_TEST(json["document_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["item_id"] = 42;
    json["document_id"] = 42;

    ItemDocument dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.itemId.has_value());
    BOOST_TEST(dto.itemId.value() == 42);
    BOOST_TEST(dto.documentId.has_value());
    BOOST_TEST(dto.documentId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ItemDocument original;

    // Поле: id
    original.id = 42;
    // Поле: itemId
    original.itemId = 42;
    // Поле: documentId
    original.documentId = 42;

    nlohmann::json json = original.toJson();
    ItemDocument deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ItemDocument dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.itemId = 42;
    dto.documentId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ItemDocument dto1;
    ItemDocument dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле itemId, чтобы сделать их разными
    dto1.itemId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    ItemDocument dto;

    dto.itemId = 42;
    dto.documentId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()