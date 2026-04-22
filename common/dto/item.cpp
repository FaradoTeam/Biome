#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item.h"

namespace dto
{

Item::Item(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Item::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор типа элемента
    if (itemTypeId.has_value())
    {
        result["item_type_id"] = itemTypeId.value();
    }
    // Идентификатор родительского элемента
    if (parentId.has_value())
    {
        result["parent_id"] = parentId.value();
    }
    // Идентификатор текущего состояния
    if (stateId.has_value())
    {
        result["state_id"] = stateId.value();
    }
    // Идентификатор фазы
    if (phaseId.has_value())
    {
        result["phase_id"] = phaseId.value();
    }
    // Название элемента
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Содержимое элемента
    if (content.has_value())
    {
        result["content"] = content.value();
    }
    // Флаг удаления (мягкое удаление)
    if (isDeleted.has_value())
    {
        result["is_deleted"] = isDeleted.value();
    }

    return result;
}

bool Item::fromJson(const nlohmann::json& json)
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
    // Идентификатор типа элемента
    if (json.contains("item_type_id") && !json["item_type_id"].is_null())
    {
        try
        {
            itemTypeId = json["item_type_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        itemTypeId = std::nullopt;
    }
    // Идентификатор родительского элемента
    if (json.contains("parent_id") && !json["parent_id"].is_null())
    {
        try
        {
            parentId = json["parent_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        parentId = std::nullopt;
    }
    // Идентификатор текущего состояния
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
    // Идентификатор фазы
    if (json.contains("phase_id") && !json["phase_id"].is_null())
    {
        try
        {
            phaseId = json["phase_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        phaseId = std::nullopt;
    }
    // Название элемента
    if (json.contains("caption") && !json["caption"].is_null())
    {
        try
        {
            caption = json["caption"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        caption = std::nullopt;
    }
    // Содержимое элемента
    if (json.contains("content") && !json["content"].is_null())
    {
        try
        {
            content = json["content"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        content = std::nullopt;
    }
    // Флаг удаления (мягкое удаление)
    if (json.contains("is_deleted") && !json["is_deleted"].is_null())
    {
        try
        {
            isDeleted = json["is_deleted"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isDeleted = std::nullopt;
    }

    return success;
}

bool Item::isValid() const
{
    if (!itemTypeId.has_value())
    {
        return false;
    }
    if (!stateId.has_value())
    {
        return false;
    }
    if (!phaseId.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }

    return true;
}

std::string Item::validationError() const
{
    if (!itemTypeId.has_value())
    {
        return "Поле «item_type_id» является обязательным для заполнения";
    }
    if (!stateId.has_value())
    {
        return "Поле «state_id» является обязательным для заполнения";
    }
    if (!phaseId.has_value())
    {
        return "Поле «phase_id» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }

    return "";
}

bool Item::operator==(const Item& other) const
{
    return
        id == other.id
        && itemTypeId == other.itemTypeId
        && parentId == other.parentId
        && stateId == other.stateId
        && phaseId == other.phaseId
        && caption == other.caption
        && content == other.content
        && isDeleted == other.isDeleted
;
}

} // namespace dto