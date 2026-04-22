#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/page_response.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(PageResponseTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    PageResponse dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.items.has_value());
    BOOST_TEST(!dto.totalCount.has_value());
    BOOST_TEST(!dto.page.has_value());
    BOOST_TEST(!dto.pageSize.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    PageResponse dto;

    // Проверка поля: items
    {
        BOOST_TEST(!dto.items.has_value());

        std::vector<std::string> testValue ={"item1", "item2"};
        dto.items = testValue;

        BOOST_TEST(dto.items.has_value());

        BOOST_TEST(dto.items.value() == testValue);

        // Проверка сброса значения
        dto.items = std::nullopt;
        BOOST_TEST(!dto.items.has_value());
    }
    // Проверка поля: totalCount
    {
        BOOST_TEST(!dto.totalCount.has_value());

        int64_t testValue =42;
        dto.totalCount = testValue;

        BOOST_TEST(dto.totalCount.has_value());

        BOOST_TEST(dto.totalCount.value() == testValue);

        // Проверка сброса значения
        dto.totalCount = std::nullopt;
        BOOST_TEST(!dto.totalCount.has_value());
    }
    // Проверка поля: page
    {
        BOOST_TEST(!dto.page.has_value());

        int64_t testValue =42;
        dto.page = testValue;

        BOOST_TEST(dto.page.has_value());

        BOOST_TEST(dto.page.value() == testValue);

        // Проверка сброса значения
        dto.page = std::nullopt;
        BOOST_TEST(!dto.page.has_value());
    }
    // Проверка поля: pageSize
    {
        BOOST_TEST(!dto.pageSize.has_value());

        int64_t testValue =42;
        dto.pageSize = testValue;

        BOOST_TEST(dto.pageSize.has_value());

        BOOST_TEST(dto.pageSize.value() == testValue);

        // Проверка сброса значения
        dto.pageSize = std::nullopt;
        BOOST_TEST(!dto.pageSize.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    PageResponse dto;

    // Поле: items
    dto.items = {"item1", "item2"};
    // Поле: totalCount
    dto.totalCount = 42;
    // Поле: page
    dto.page = 42;
    // Поле: pageSize
    dto.pageSize = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("items"));
    BOOST_TEST(json["items"].is_array());
    BOOST_TEST(json["items"].size() == 2);
    BOOST_TEST(json.contains("total_count"));
    BOOST_TEST(json["total_count"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("page"));
    BOOST_TEST(json["page"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("page_size"));
    BOOST_TEST(json["page_size"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["items"] = nlohmann::json::array({"item1", "item2"});
    json["total_count"] = 42;
    json["page"] = 42;
    json["page_size"] = 42;

    PageResponse dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.items.has_value());
    BOOST_TEST(dto.items.value().size() == 2);
    BOOST_TEST(dto.totalCount.has_value());
    BOOST_TEST(dto.totalCount.value() == 42);
    BOOST_TEST(dto.page.has_value());
    BOOST_TEST(dto.page.value() == 42);
    BOOST_TEST(dto.pageSize.has_value());
    BOOST_TEST(dto.pageSize.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    PageResponse original;

    // Поле: items
    original.items = {"item1", "item2"};
    // Поле: totalCount
    original.totalCount = 42;
    // Поле: page
    original.page = 42;
    // Поле: pageSize
    original.pageSize = 42;

    nlohmann::json json = original.toJson();
    PageResponse deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    PageResponse dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());

    // Заполняем обязательные поля

    // Теперь должен быть валидным
    // Если обязательных полей нет, DTO всегда валиден
    BOOST_TEST(dto.isValid());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    PageResponse dto1;
    PageResponse dto2;

    // Изначально они должны быть равны (оба пустые)
    BOOST_TEST(dto1 == dto2);
    BOOST_TEST(!(dto1 != dto2));


    // Если нет обязательных полей, используем первое неконстантное поле
    dto1.items = {"different"};

    BOOST_TEST(dto1 != dto2);
    BOOST_TEST(!(dto1 == dto2));
}

// Тест: Оператор потокового вывода
BOOST_AUTO_TEST_CASE(StreamOutput)
{
    PageResponse dto;


    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()