#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item_history.h"

namespace dto
{

ItemHistory::ItemHistory(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemHistory::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор элемента
    if (itemId.has_value())
    {
        result["item_id"] = itemId.value();
    }
    // Идентификатор пользователя
    if (userId.has_value())
    {
        result["user_id"] = userId.value();
    }
    // Время изменения
    if (timestamp.has_value())
    {
        result["timestamp"] = timePointToSeconds(timestamp.value());
    }
    // JSON с изменениями (diff)
    if (diff.has_value())
    {
        result["diff"] = diff.value();
    }

    return result;
}

bool ItemHistory::fromJson(const nlohmann::json& json)
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
    // Идентификатор элемента
    if (json.contains("item_id") && !json["item_id"].is_null())
    {
        try
        {
            itemId = json["item_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        itemId = std::nullopt;
    }
    // Идентификатор пользователя
    if (json.contains("user_id") && !json["user_id"].is_null())
    {
        try
        {
            userId = json["user_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        userId = std::nullopt;
    }
    // Время изменения
    if (json.contains("timestamp") && !json["timestamp"].is_null())
    {
        try
        {
            auto timestampValue = json["timestamp"].get<int64_t>();
            timestamp = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        timestamp = std::nullopt;
    }
    // JSON с изменениями (diff)
    if (json.contains("diff") && !json["diff"].is_null())
    {
        try
        {
            diff = json["diff"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        diff = std::nullopt;
    }

    return success;
}

bool ItemHistory::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!userId.has_value())
    {
        return false;
    }
    if (!timestamp.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ItemHistory::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!timestamp.has_value())
    {
        return "Поле «timestamp» является обязательным для заполнения";
    }


    return "";
}

bool ItemHistory::operator==(const ItemHistory& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && userId == other.userId
        && timestamp == other.timestamp
        && diff == other.diff
;
}

} // namespace dto