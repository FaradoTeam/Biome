#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "edge.h"

namespace dto
{

Edge::Edge(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Edge::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Начальное состояние
    if (beginStateId.has_value())
    {
        result["begin_state_id"] = beginStateId.value();
    }
    // Конечное состояние
    if (endStateId.has_value())
    {
        result["end_state_id"] = endStateId.value();
    }

    return result;
}

bool Edge::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // Уникальный идентификатор
    if (json.contains("id") && !json["id"].is_null())
    {
        try
        {
            id = json["id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        id = std::nullopt;
    }
    // Начальное состояние
    if (json.contains("begin_state_id") && !json["begin_state_id"].is_null())
    {
        try
        {
            beginStateId = json["begin_state_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        beginStateId = std::nullopt;
    }
    // Конечное состояние
    if (json.contains("end_state_id") && !json["end_state_id"].is_null())
    {
        try
        {
            endStateId = json["end_state_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        endStateId = std::nullopt;
    }

    return success;
}

bool Edge::isValid() const
{
    if (!beginStateId.has_value())
    {
        return false;
    }
    if (!endStateId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string Edge::validationError() const
{
    if (!beginStateId.has_value())
    {
        return "Поле «begin_state_id» является обязательным для заполнения";
    }
    if (!endStateId.has_value())
    {
        return "Поле «end_state_id» является обязательным для заполнения";
    }


    return "";
}

bool Edge::operator==(const Edge& other) const
{
    return
        id == other.id
        && beginStateId == other.beginStateId
        && endStateId == other.endStateId
;
}

} // namespace dto