#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "common/dto/project_team.h"

#include <optional>
#include "common/helpers/time_helpers.h"

using namespace dto;

BOOST_AUTO_TEST_SUITE(ProjectTeamTests)

// Тест: Конструктор по умолчанию
BOOST_AUTO_TEST_CASE(DefaultConstructor)
{
    ProjectTeam dto;

    // Все optional поля должны быть пустыми
    BOOST_TEST(!dto.id.has_value());
    BOOST_TEST(!dto.projectId.has_value());
    BOOST_TEST(!dto.teamId.has_value());
}

// Тест: Прямой доступ к полям
BOOST_AUTO_TEST_CASE(FieldAccess)
{
    ProjectTeam dto;

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
    // Проверка поля: teamId
    {
        BOOST_TEST(!dto.teamId.has_value());

        int64_t testValue =42;
        dto.teamId = testValue;

        BOOST_TEST(dto.teamId.has_value());

        BOOST_TEST(dto.teamId.value() == testValue);

        // Проверка сброса значения
        dto.teamId = std::nullopt;
        BOOST_TEST(!dto.teamId.has_value());
    }
}

// Тест: сериализация в JSON
BOOST_AUTO_TEST_CASE(ToJsonSerialization)
{
    ProjectTeam dto;

    // Поле: id
    dto.id = 42;
    // Поле: projectId
    dto.projectId = 42;
    // Поле: teamId
    dto.teamId = 42;

    nlohmann::json json = dto.toJson();

    // Проверка полей JSON
    BOOST_TEST(json.contains("id"));
    BOOST_TEST(json["id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("project_id"));
    BOOST_TEST(json["project_id"].get<int64_t>() == 42);
    BOOST_TEST(json.contains("team_id"));
    BOOST_TEST(json["team_id"].get<int64_t>() == 42);
}

// Тест: десериализация из JSON
BOOST_AUTO_TEST_CASE(FromJsonDeserialization)
{
    nlohmann::json json = nlohmann::json::object();
    json["id"] = 42;
    json["project_id"] = 42;
    json["team_id"] = 42;

    ProjectTeam dto(json);

    // Проверка десериализованных значений
    BOOST_TEST(dto.id.has_value());
    BOOST_TEST(dto.id.value() == 42);
    BOOST_TEST(dto.projectId.has_value());
    BOOST_TEST(dto.projectId.value() == 42);
    BOOST_TEST(dto.teamId.has_value());
    BOOST_TEST(dto.teamId.value() == 42);
}

// Тест: Сериализация в оба конца
BOOST_AUTO_TEST_CASE(RoundTripSerialization)
{
    ProjectTeam original;

    // Поле: id
    original.id = 42;
    // Поле: projectId
    original.projectId = 42;
    // Поле: teamId
    original.teamId = 42;

    nlohmann::json json = original.toJson();
    ProjectTeam deserialized(json);

    // Сравнение исходного и десериализованного
    BOOST_TEST(original == deserialized);
}

// Тест: проверка isValid
BOOST_AUTO_TEST_CASE(Validation)
{
    ProjectTeam dto;

    // Проверяем, есть ли обязательные поля

    // Изначально невалиден, если есть обязательные поля
    BOOST_TEST(!dto.isValid());
    BOOST_TEST(dto.validationError().find("обязательным") != std::string::npos);

    // Заполняем обязательные поля
    dto.projectId = 42;
    dto.teamId = 42;

    // Теперь должен быть валидным
    BOOST_TEST(dto.isValid());
    BOOST_TEST(dto.validationError().empty());
}

// Тест: Операторы сравнения
BOOST_AUTO_TEST_CASE(ComparisonOperators)
{
    ProjectTeam dto1;
    ProjectTeam dto2;

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
    ProjectTeam dto;

    dto.projectId = 42;
    dto.teamId = 42;

    std::stringstream ss;
    ss << dto;
    BOOST_TEST(!ss.str().empty());
}

BOOST_AUTO_TEST_SUITE_END()