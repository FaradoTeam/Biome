#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/phase.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(PhaseTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Phase dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.projectId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.beginDate.has_value());
    BOOST_TEST(!dto.endDate.has_value());
    BOOST_TEST(!dto.isArchive.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Phase dto;

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
    // Проверка поля: projectId
    {
        BOOST_TEST(!dto.projectId.has_value());

        int64_t testValue =42;
        dto.projectId = testValue;

        BOOST_TEST(dto.projectId.has_value());

        BOOST_TEST(dto.projectId.value() == testValue);

        // Проверка сброса значения
        dto.projectId = std::nullopt;
        BOOST_TEST(!dto.projectId.has_value());
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
    // Проверка поля: beginDate
    {
        BOOST_TEST(!dto.beginDate.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.beginDate = testValue;

        BOOST_TEST(dto.beginDate.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.beginDate.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.beginDate = std::nullopt;
        BOOST_TEST(!dto.beginDate.has_value());
    }
    // Проверка поля: endDate
    {
        BOOST_TEST(!dto.endDate.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.endDate = testValue;

        BOOST_TEST(dto.endDate.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.endDate.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.endDate = std::nullopt;
        BOOST_TEST(!dto.endDate.has_value());
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
    Phase dto;

    // Поле: id
    dto.id = 42;
    // Поле: projectId
    dto.projectId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: beginDate
    dto.beginDate = secondsToTimePoint(1640995200);
    // Поле: endDate
    dto.endDate = secondsToTimePoint(1640995200);
    // Поле: isArchive
    dto.isArchive = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("project_id"));
    BOOST_TEST(json["project_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("begin_date"));
    BOOST_TEST(json["begin_date"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("end_date"));
    BOOST_TEST(json["end_date"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("is_archive"));
    BOOST_TEST(json["is_archive"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["project_id"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["begin_date"] = 1640995200;
    json["end_date"] = 1640995200;
    json["is_archive"] = true;

    Phase dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.projectId.has_value());
    BOOST_TEST(dto.projectId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.beginDate.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.beginDate.value()), 1640995200);
    BOOST_TEST(dto.endDate.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.endDate.value()), 1640995200);
    BOOST_TEST(dto.isArchive.has_value());
    BOOST_TEST(dto.isArchive.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Phase original;

    // Поле: id
    original.id = 42;
    // Поле: projectId
    original.projectId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: beginDate
    original.beginDate = secondsToTimePoint(1640995200);
    // Поле: endDate
    original.endDate = secondsToTimePoint(1640995200);
    // Поле: isArchive
    original.isArchive = true;

    nlohmann::json json = original.toJson();
    Phase deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Phase dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.projectId = 42;
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Phase dto1;
    Phase dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле projectId, чтобы сделать их разными
    dto1.projectId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    Phase dto;

    dto.projectId = 42;
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()