#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/plan.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(PlanTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Plan dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.phaseId.has_value());
    BOOST_TEST(!dto.basePlanId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.isActive.has_value());
    BOOST_TEST(!dto.createdAt.has_value());
    BOOST_TEST(!dto.createdByUserId.has_value());
    BOOST_TEST(!dto.activatedAt.has_value());
    BOOST_TEST(!dto.activatedByUserId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Plan dto;

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
    // Проверка поля: basePlanId
    {
        BOOST_TEST(!dto.basePlanId.has_value());

        int64_t testValue =42;
        dto.basePlanId = testValue;

        BOOST_TEST(dto.basePlanId.has_value());

        BOOST_TEST(dto.basePlanId.value() == testValue);

        // Проверка сброса значения
        dto.basePlanId = std::nullopt;
        BOOST_TEST(!dto.basePlanId.has_value());
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
    // Проверка поля: isActive
    {
        BOOST_TEST(!dto.isActive.has_value());

        bool testValue =true;
        dto.isActive = testValue;

        BOOST_TEST(dto.isActive.has_value());

        BOOST_TEST(dto.isActive.value() == testValue);

        // Проверка сброса значения
        dto.isActive = std::nullopt;
        BOOST_TEST(!dto.isActive.has_value());
    }
    // Проверка поля: createdAt
    {
        BOOST_TEST(!dto.createdAt.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.createdAt = testValue;

        BOOST_TEST(dto.createdAt.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.createdAt.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.createdAt = std::nullopt;
        BOOST_TEST(!dto.createdAt.has_value());
    }
    // Проверка поля: createdByUserId
    {
        BOOST_TEST(!dto.createdByUserId.has_value());

        int64_t testValue =42;
        dto.createdByUserId = testValue;

        BOOST_TEST(dto.createdByUserId.has_value());

        BOOST_TEST(dto.createdByUserId.value() == testValue);

        // Проверка сброса значения
        dto.createdByUserId = std::nullopt;
        BOOST_TEST(!dto.createdByUserId.has_value());
    }
    // Проверка поля: activatedAt
    {
        BOOST_TEST(!dto.activatedAt.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.activatedAt = testValue;

        BOOST_TEST(dto.activatedAt.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.activatedAt.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.activatedAt = std::nullopt;
        BOOST_TEST(!dto.activatedAt.has_value());
    }
    // Проверка поля: activatedByUserId
    {
        BOOST_TEST(!dto.activatedByUserId.has_value());

        int64_t testValue =42;
        dto.activatedByUserId = testValue;

        BOOST_TEST(dto.activatedByUserId.has_value());

        BOOST_TEST(dto.activatedByUserId.value() == testValue);

        // Проверка сброса значения
        dto.activatedByUserId = std::nullopt;
        BOOST_TEST(!dto.activatedByUserId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Plan dto;

    // Поле: id
    dto.id = 42;
    // Поле: phaseId
    dto.phaseId = 42;
    // Поле: basePlanId
    dto.basePlanId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: isActive
    dto.isActive = true;
    // Поле: createdAt
    dto.createdAt = secondsToTimePoint(1640995200);
    // Поле: createdByUserId
    dto.createdByUserId = 42;
    // Поле: activatedAt
    dto.activatedAt = secondsToTimePoint(1640995200);
    // Поле: activatedByUserId
    dto.activatedByUserId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("phaseId"));
    BOOST_TEST(json["phaseId"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("basePlanId"));
    BOOST_TEST(json["basePlanId"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("isActive"));
    BOOST_TEST(json["isActive"].get<bool>() == true);
    BOOST_TEST(json.contains("createdAt"));
    BOOST_TEST(json["createdAt"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("createdByUserId"));
    BOOST_TEST(json["createdByUserId"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("activatedAt"));
    BOOST_TEST(json["activatedAt"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("activatedByUserId"));
    BOOST_TEST(json["activatedByUserId"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["phaseId"] = 42;
    json["basePlanId"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["isActive"] = true;
    json["createdAt"] = 1640995200;
    json["createdByUserId"] = 42;
    json["activatedAt"] = 1640995200;
    json["activatedByUserId"] = 42;

    Plan dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.phaseId.has_value());
    BOOST_TEST(dto.phaseId.value() == 42);
    BOOST_TEST(dto.basePlanId.has_value());
    BOOST_TEST(dto.basePlanId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.isActive.has_value());
    BOOST_TEST(dto.isActive.value() == true);
    BOOST_TEST(dto.createdAt.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.createdAt.value()), 1640995200);
    BOOST_TEST(dto.createdByUserId.has_value());
    BOOST_TEST(dto.createdByUserId.value() == 42);
    BOOST_TEST(dto.activatedAt.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.activatedAt.value()), 1640995200);
    BOOST_TEST(dto.activatedByUserId.has_value());
    BOOST_TEST(dto.activatedByUserId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Plan original;

    // Поле: id
    original.id = 42;
    // Поле: phaseId
    original.phaseId = 42;
    // Поле: basePlanId
    original.basePlanId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: isActive
    original.isActive = true;
    // Поле: createdAt
    original.createdAt = secondsToTimePoint(1640995200);
    // Поле: createdByUserId
    original.createdByUserId = 42;
    // Поле: activatedAt
    original.activatedAt = secondsToTimePoint(1640995200);
    // Поле: activatedByUserId
    original.activatedByUserId = 42;

    nlohmann::json json = original.toJson();
    Plan deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Plan dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.phaseId = 42;
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    Plan dto1;
    Plan dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле phaseId, чтобы сделать их разными
    dto1.phaseId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    Plan dto;

    dto.phaseId = 42;
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()