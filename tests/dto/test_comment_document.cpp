#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/comment_document.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(CommentDocumentTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    CommentDocument dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.commentId.has_value());
    BOOST_TEST(!dto.documentId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    CommentDocument dto;

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
    // Проверка поля: commentId
    {
        BOOST_TEST(!dto.commentId.has_value());

        int64_t testValue =42;
        dto.commentId = testValue;

        BOOST_TEST(dto.commentId.has_value());

        BOOST_TEST(dto.commentId.value() == testValue);

        // Проверка сброса значения
        dto.commentId = std::nullopt;
        BOOST_TEST(!dto.commentId.has_value());
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
    CommentDocument dto;

    // Поле: id
    dto.id = 42;
    // Поле: commentId
    dto.commentId = 42;
    // Поле: documentId
    dto.documentId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("comment_id"));
    BOOST_TEST(json["comment_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("document_id"));
    BOOST_TEST(json["document_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["comment_id"] = 42;
    json["document_id"] = 42;

    CommentDocument dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.commentId.has_value());
    BOOST_TEST(dto.commentId.value() == 42);
    BOOST_TEST(dto.documentId.has_value());
    BOOST_TEST(dto.documentId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    CommentDocument original;

    // Поле: id
    original.id = 42;
    // Поле: commentId
    original.commentId = 42;
    // Поле: documentId
    original.documentId = 42;

    nlohmann::json json = original.toJson();
    CommentDocument deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    CommentDocument dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.commentId = 42;
    dto.documentId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    CommentDocument dto1;
    CommentDocument dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));

    // Изменим поле commentId, чтобы сделать их разными
    dto1.commentId = 999;

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));

}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    CommentDocument dto;

    dto.commentId = 42;
    dto.documentId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()