#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/types.h"

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
        result["beginStateId"] = beginStateId.value();
    }
    // Конечное состояние
    if (endStateId.has_value())
    {
        result["endStateId"] = endStateId.value();
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
    if (json.contains("beginStateId") && !json["beginStateId"].is_null())
    {
        try
        {
            beginStateId = json["beginStateId"].get<int64_t>();
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
    if (json.contains("endStateId") && !json["endStateId"].is_null())
    {
        try
        {
            endStateId = json["endStateId"].get<int64_t>();
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
        return "Поле «beginStateId» является обязательным для заполнения";
    }
    if (!endStateId.has_value())
    {
        return "Поле «endStateId» является обязательным для заполнения";
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