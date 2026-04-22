#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/document.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(DocumentTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    Document dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.caption.has_value());
    BOOST_TEST(!dto.description.has_value());
    BOOST_TEST(!dto.path.has_value());
    BOOST_TEST(!dto.filename.has_value());
    BOOST_TEST(!dto.size.has_value());
    BOOST_TEST(!dto.mimeType.has_value());
    BOOST_TEST(!dto.uploadedAt.has_value());
    BOOST_TEST(!dto.uploadedByUserId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    Document dto;

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
    // Проверка поля: path
    {
        BOOST_TEST(!dto.path.has_value());

        std::string testValue ="test_value";
        dto.path = testValue;

        BOOST_TEST(dto.path.has_value());

        BOOST_TEST(dto.path.value() == testValue);

        // Проверка сброса значения
        dto.path = std::nullopt;
        BOOST_TEST(!dto.path.has_value());
    }
    // Проверка поля: filename
    {
        BOOST_TEST(!dto.filename.has_value());

        std::string testValue ="test_value";
        dto.filename = testValue;

        BOOST_TEST(dto.filename.has_value());

        BOOST_TEST(dto.filename.value() == testValue);

        // Проверка сброса значения
        dto.filename = std::nullopt;
        BOOST_TEST(!dto.filename.has_value());
    }
    // Проверка поля: size
    {
        BOOST_TEST(!dto.size.has_value());

        int64_t testValue =42;
        dto.size = testValue;

        BOOST_TEST(dto.size.has_value());

        BOOST_TEST(dto.size.value() == testValue);

        // Проверка сброса значения
        dto.size = std::nullopt;
        BOOST_TEST(!dto.size.has_value());
    }
    // Проверка поля: mimeType
    {
        BOOST_TEST(!dto.mimeType.has_value());

        std::string testValue ="test_value";
        dto.mimeType = testValue;

        BOOST_TEST(dto.mimeType.has_value());

        BOOST_TEST(dto.mimeType.value() == testValue);

        // Проверка сброса значения
        dto.mimeType = std::nullopt;
        BOOST_TEST(!dto.mimeType.has_value());
    }
    // Проверка поля: uploadedAt
    {
        BOOST_TEST(!dto.uploadedAt.has_value());

        std::chrono::system_clock::time_point testValue =secondsToTimePoint(1640995200);
        dto.uploadedAt = testValue;

        BOOST_TEST(dto.uploadedAt.has_value());

        BOOST_CHECK_EQUAL(
            timePointToSeconds(dto.uploadedAt.value()),
            timePointToSeconds(testValue)
        );

        // Проверка сброса значения
        dto.uploadedAt = std::nullopt;
        BOOST_TEST(!dto.uploadedAt.has_value());
    }
    // Проверка поля: uploadedByUserId
    {
        BOOST_TEST(!dto.uploadedByUserId.has_value());

        int64_t testValue =42;
        dto.uploadedByUserId = testValue;

        BOOST_TEST(dto.uploadedByUserId.has_value());

        BOOST_TEST(dto.uploadedByUserId.value() == testValue);

        // Проверка сброса значения
        dto.uploadedByUserId = std::nullopt;
        BOOST_TEST(!dto.uploadedByUserId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    Document dto;

    // Поле: id
    dto.id = 42;
    // Поле: caption
    dto.caption = "test_caption";
    // Поле: description
    dto.description = "test_description";
    // Поле: path
    dto.path = "test_path";
    // Поле: filename
    dto.filename = "test_filename";
    // Поле: size
    dto.size = 42;
    // Поле: mimeType
    dto.mimeType = "test_mime_type";
    // Поле: uploadedAt
    dto.uploadedAt = secondsToTimePoint(1640995200);
    // Поле: uploadedByUserId
    dto.uploadedByUserId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("caption"));
    BOOST_TEST(json["caption"].get<std::string>() == "test_caption");
    BOOST_TEST(json.contains("description"));
    BOOST_TEST(json["description"].get<std::string>() == "test_description");
    BOOST_TEST(json.contains("path"));
    BOOST_TEST(json["path"].get<std::string>() == "test_path");
    BOOST_TEST(json.contains("filename"));
    BOOST_TEST(json["filename"].get<std::string>() == "test_filename");
    BOOST_TEST(json.contains("size"));
    BOOST_TEST(json["size"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("mime_type"));
    BOOST_TEST(json["mime_type"].get<std::string>() == "test_mime_type");
    BOOST_TEST(json.contains("uploaded_at"));
    BOOST_TEST(json["uploaded_at"].get<int64_t>() == 1640995200);
    BOOST_TEST(json.contains("uploaded_by_user_id"));
    BOOST_TEST(json["uploaded_by_user_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["caption"] = "test_caption";
    json["description"] = "test_description";
    json["path"] = "test_path";
    json["filename"] = "test_filename";
    json["size"] = 42;
    json["mime_type"] = "test_mime_type";
    json["uploaded_at"] = 1640995200;
    json["uploaded_by_user_id"] = 42;

    Document dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.caption.has_value());
    BOOST_TEST(dto.caption.value() == "test_caption");
    BOOST_TEST(dto.description.has_value());
    BOOST_TEST(dto.description.value() == "test_description");
    BOOST_TEST(dto.path.has_value());
    BOOST_TEST(dto.path.value() == "test_path");
    BOOST_TEST(dto.filename.has_value());
    BOOST_TEST(dto.filename.value() == "test_filename");
    BOOST_TEST(dto.size.has_value());
    BOOST_TEST(dto.size.value() == 42);
    BOOST_TEST(dto.mimeType.has_value());
    BOOST_TEST(dto.mimeType.value() == "test_mime_type");
    BOOST_TEST(dto.uploadedAt.has_value());
    BOOST_CHECK_EQUAL(timePointToSeconds(dto.uploadedAt.value()), 1640995200);
    BOOST_TEST(dto.uploadedByUserId.has_value());
    BOOST_TEST(dto.uploadedByUserId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    Document original;

    // Поле: id
    original.id = 42;
    // Поле: caption
    original.caption = "test_caption";
    // Поле: description
    original.description = "test_description";
    // Поле: path
    original.path = "test_path";
    // Поле: filename
    original.filename = "test_filename";
    // Поле: size
    original.size = 42;
    // Поле: mimeType
    original.mimeType = "test_mime_type";
    // Поле: uploadedAt
    original.uploadedAt = secondsToTimePoint(1640995200);
    // Поле: uploadedByUserId
    original.uploadedByUserId = 42;

    nlohmann::json json = original.toJson();
    Document deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    Document dto;

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
    Document dto1;
    Document dto2;

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
    Document dto;

    dto.caption = "test_value";

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()