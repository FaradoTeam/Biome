#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item_user_state.h"

namespace dto
{

ItemUserState::ItemUserState(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemUserState::toJson() const
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
    // Идентификатор состояния
    if (stateId.has_value())
    {
        result["state_id"] = stateId.value();
    }
    // Комментарий к переходу
    if (comment.has_value())
    {
        result["comment"] = comment.value();
    }
    // Время перехода
    if (timestamp.has_value())
    {
        result["timestamp"] = timePointToSeconds(timestamp.value());
    }

    return result;
}

bool ItemUserState::fromJson(const nlohmann::json& json)
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
    // Идентификатор состояния
    if (json.contains("state_id") && !json["state_id"].is_null())
    {
        try
        {
            stateId = json["state_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        stateId = std::nullopt;
    }
    // Комментарий к переходу
    if (json.contains("comment") && !json["comment"].is_null())
    {
        try
        {
            comment = json["comment"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        comment = std::nullopt;
    }
    // Время перехода
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

    return success;
}

bool ItemUserState::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!userId.has_value())
    {
        return false;
    }
    if (!stateId.has_value())
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

std::string ItemUserState::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }
    if (!userId.has_value())
    {
        return "Поле «user_id» является обязательным для заполнения";
    }
    if (!stateId.has_value())
    {
        return "Поле «state_id» является обязательным для заполнения";
    }
    if (!timestamp.has_value())
    {
        return "Поле «timestamp» является обязательным для заполнения";
    }


    return "";
}

bool ItemUserState::operator==(const ItemUserState& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && userId == other.userId
        && stateId == other.stateId
        && comment == other.comment
        && timestamp == other.timestamp
;
}

} // namespace dto