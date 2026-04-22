#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/state.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(StateTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    State dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.workflowId.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.weight.has_value());
    BOOST_TEST(!dto.orderNumber.has_value());
    BOOST_TEST(!dto.isArchive.has_value());
    BOOST_TEST(!dto.isQueue.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    State dto;

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
    // Проверка поля: weight
    {
        BOOST_TEST(!dto.weight.has_value());

        int64_t testValue =42;
        dto.weight = testValue;

        BOOST_TEST(dto.weight.has_value());

        BOOST_TEST(dto.weight.value() == testValue);

        // Проверка сброса значения
        dto.weight = std::nullopt;
        BOOST_TEST(!dto.weight.has_value());
    }
    // Проверка поля: orderNumber
    {
        BOOST_TEST(!dto.orderNumber.has_value());

        int64_t testValue =42;
        dto.orderNumber = testValue;

        BOOST_TEST(dto.orderNumber.has_value());

        BOOST_TEST(dto.orderNumber.value() == testValue);

        // Проверка сброса значения
        dto.orderNumber = std::nullopt;
        BOOST_TEST(!dto.orderNumber.has_value());
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
    // Проверка поля: isQueue
    {
        BOOST_TEST(!dto.isQueue.has_value());

        bool testValue =true;
        dto.isQueue = testValue;

        BOOST_TEST(dto.isQueue.has_value());

        BOOST_TEST(dto.isQueue.value() == testValue);

        // Проверка сброса значения
        dto.isQueue = std::nullopt;
        BOOST_TEST(!dto.isQueue.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    State dto;

    // Поле: id
    dto.id = 42;
    // Поле: workflowId
    dto.workflowId = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: weight
    dto.weight = 42;
    // Поле: orderNumber
    dto.orderNumber = 42;
    // Поле: isArchive
    dto.isArchive = true;
    // Поле: isQueue
    dto.isQueue = true;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("workflow_id"));
    BOOST_TEST(json["workflow_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("weight"));
    BOOST_TEST(json["weight"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("order_number"));
    BOOST_TEST(json["order_number"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("is_archive"));
    BOOST_TEST(json["is_archive"].get<bool>() == true);
    BOOST_TEST(json.contains("is_queue"));
    BOOST_TEST(json["is_queue"].get<bool>() == true);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["workflow_id"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["weight"] = 42;
    json["order_number"] = 42;
    json["is_archive"] = true;
    json["is_queue"] = true;

    State dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.workflowId.has_value());
    BOOST_TEST(dto.workflowId.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.weight.has_value());
    BOOST_TEST(dto.weight.value() == 42);
    BOOST_TEST(dto.orderNumber.has_value());
    BOOST_TEST(dto.orderNumber.value() == 42);
    BOOST_TEST(dto.isArchive.has_value());
    BOOST_TEST(dto.isArchive.value() == true);
    BOOST_TEST(dto.isQueue.has_value());
    BOOST_TEST(dto.isQueue.value() == true);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    State original;

    // Поле: id
    original.id = 42;
    // Поле: workflowId
    original.workflowId = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: weight
    original.weight = 42;
    // Поле: orderNumber
    original.orderNumber = 42;
    // Поле: isArchive
    original.isArchive = true;
    // Поле: isQueue
    original.isQueue = true;

    nlohmann::json json = original.toJson();
    State deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    State dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.workflowId = 42;
    dto.caption = "test_caption";

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    State dto1;
    State dto2;

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
    State dto;

    dto.workflowId = 42;
    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()