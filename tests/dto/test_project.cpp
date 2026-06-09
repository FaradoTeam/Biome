#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/project.h"

#include <optional>

#include "common/types.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ProjectTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Project dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.parentId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.createdAt.has_value());
    BOOST_TEST(!dto.updatedAt.has_value());
    BOOST_TEST(!dto.isArchive.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Project dto;

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
    // Проверка поля: createdAt
    {
        BOOST_TEST(!dto.createdAt.has_value());

        std::chrono::system_clock::time_point testValue =common::secondsToTimePoint(1640995200);
        dto.createdAt = testValue;

        BOOST_TEST(dto.createdAt.has_value());

        BOOST_CHECK_EQUAL(
            common::timePointToSeconds(dto.createdAt.value()),
            common::timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.createdAt = std::nullopt;
        BOOST_TEST(!dto.createdAt.has_value());
    }
    // Проверка поля: updatedAt
    {
        BOOST_TEST(!dto.updatedAt.has_value());

        std::chrono::system_clock::time_point testValue =common::secondsToTimePoint(1640995200);
        dto.updatedAt = testValue;

        BOOST_TEST(dto.updatedAt.has_value());

        BOOST_CHECK_EQUAL(
            common::timePointToSeconds(dto.updatedAt.value()),
            common::timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.updatedAt = std::nullopt;
        BOOST_TEST(!dto.updatedAt.has_value());
    }
    // Проверка поля: isArchive
    {
        BOOST_TEST(!dto.isArchive.has_value());

        bool testValue =true;
        dto.isArchive = testValue;

        BOOST_TEST(dto.isArchive.has_value());

        BOOST_TEST(dto.isArchive.value() == testValue);

        // Проверка сброса значения
        dto.isArchive = std::nullopt;
        BOOST_TEST(!dto.isArchive.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Project dto;

    // Поле: id
    dto.id = 42;
    // Поле: parentId
    dto.parentId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: createdAt
    dto.createdAt = common::secondsToTimePoint(1640995200);
    // Поле: updatedAt
    dto.updatedAt = common::secondsToTimePoint(1640995200);
    // Поле: isArchive
    dto.isArchive = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("parentId"));
    BOOST_TEST(json["parentId"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("createdAt"));
    BOOST_TEST(json["createdAt"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("updatedAt"));
    BOOST_TEST(json["updatedAt"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("isArchive"));
    BOOST_TEST(json["isArchive"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["parentId"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["createdAt"] = 1640995200;
    json["updatedAt"] = 1640995200;
    json["isArchive"] = true;

    Project dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.parentId.has_value());
    BOOST_TEST(dto.parentId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.createdAt.has_value());
    BOOST_CHECK_EQUAL(common::timePointToSeconds(dto.createdAt.value()), 1640995200);
    BOOST_TEST(dto.updatedAt.has_value());
    BOOST_CHECK_EQUAL(common::timePointToSeconds(dto.updatedAt.value()), 1640995200);
    BOOST_TEST(dto.isArchive.has_value());
    BOOST_TEST(dto.isArchive.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Project original;

    // Поле: id
    original.id = 42;
    // Поле: parentId
    original.parentId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: createdAt
    original.createdAt = common::secondsToTimePoint(1640995200);
    // Поле: updatedAt
    original.updatedAt = common::secondsToTimePoint(1640995200);
    // Поле: isArchive
    original.isArchive = true;

    nlohmann::json json = original.toJson();
    Project deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Project dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Project dto1;
    Project dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле caption, чтобы сделать их разными
    dto1.caption = "different_value";

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    Project dto;

    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()