#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "board_column.h"

namespace dto
{

BoardColumn::BoardColumn(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json BoardColumn::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор доски
    if (boardId.has_value())
    {
        result["board_id"] = boardId.value();
    }
    // Идентификатор состояния
    if (stateId.has_value())
    {
        result["state_id"] = stateId.value();
    }
    // Порядковый номер колонки на доске
    if (orderNumber.has_value())
    {
        result["order_number"] = orderNumber.value();
    }
    // JSON с настройками (WIP limit
    if (settings.has_value())
    {
        result["settings"] = settings.value();
    }

    return result;
}

bool BoardColumn::fromJson(const nlohmann::json& json)
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
    // Идентификатор доски
    if (json.contains("board_id") && !json["board_id"].is_null())
    {
        try
        {
            boardId = json["board_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        boardId = std::nullopt;
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
    // Порядковый номер колонки на доске
    if (json.contains("order_number") && !json["order_number"].is_null())
    {
        try
        {
            orderNumber = json["order_number"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        orderNumber = std::nullopt;
    }
    // JSON с настройками (WIP limit
    if (json.contains("settings") && !json["settings"].is_null())
    {
        try
        {
            settings = json["settings"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        settings = std::nullopt;
    }

    return success;
}

bool BoardColumn::isValid() const
{
    if (!boardId.has_value())
    {
        return false;
    }
    if (!stateId.has_value())
    {
        return false;
    }
    if (!orderNumber.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string BoardColumn::validationError() const
{
    if (!boardId.has_value())
    {
        return "Поле «board_id» является обязательным для заполнения";
    }
    if (!stateId.has_value())
    {
        return "Поле «state_id» является обязательным для заполнения";
    }
    if (!orderNumber.has_value())
    {
        return "Поле «order_number» является обязательным для заполнения";
    }


    return "";
}

bool BoardColumn::operator==(const BoardColumn& other) const
{
    return
        id == other.id
        && boardId == other.boardId
        && stateId == other.stateId
        && orderNumber == other.orderNumber
        && settings == other.settings
;
}

} // namespace dto