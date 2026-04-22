#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item_type.h"

namespace dto
{

ItemType::ItemType(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemType::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор рабочего процесса
    if (workflowId.has_value())
    {
        result["workflow_id"] = workflowId.value();
    }
    // Состояние по умолчанию для новых элементов
    if (defaultStateId.has_value())
    {
        result["default_state_id"] = defaultStateId.value();
    }
    // Название типа
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Вид элемента
    if (kind.has_value())
    {
        result["kind"] = kind.value();
    }
    // Содержимое по умолчанию (шаблон для создания новых item)
    if (defaultContent.has_value())
    {
        result["default_content"] = defaultContent.value();
    }

    return result;
}

bool ItemType::fromJson(const nlohmann::json& json)
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
    // Идентификатор рабочего процесса
    if (json.contains("workflow_id") && !json["workflow_id"].is_null())
    {
        try
        {
            workflowId = json["workflow_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        workflowId = std::nullopt;
    }
    // Состояние по умолчанию для новых элементов
    if (json.contains("default_state_id") && !json["default_state_id"].is_null())
    {
        try
        {
            defaultStateId = json["default_state_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        defaultStateId = std::nullopt;
    }
    // Название типа
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
    // Вид элемента
    if (json.contains("kind") && !json["kind"].is_null())
    {
        try
        {
            kind = json["kind"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        kind = std::nullopt;
    }
    // Содержимое по умолчанию (шаблон для создания новых item)
    if (json.contains("default_content") && !json["default_content"].is_null())
    {
        try
        {
            defaultContent = json["default_content"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        defaultContent = std::nullopt;
    }

    return success;
}

bool ItemType::isValid() const
{
    if (!workflowId.has_value())
    {
        return false;
    }
    if (!defaultStateId.has_value())
    {
        return false;
    }
    if (!caption.has_value())
    {
        return false;
    }
    if (!kind.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений
    if (caption.value().empty())
    {
        return false;
    }
    if (kind.value().empty())
    {
        return false;
    }

    return true;
}

std::string ItemType::validationError() const
{
    if (!workflowId.has_value())
    {
        return "Поле «workflow_id» является обязательным для заполнения";
    }
    if (!defaultStateId.has_value())
    {
        return "Поле «default_state_id» является обязательным для заполнения";
    }
    if (!caption.has_value())
    {
        return "Поле «caption» является обязательным для заполнения";
    }
    if (!kind.has_value())
    {
        return "Поле «kind» является обязательным для заполнения";
    }

    if (caption.value().empty())
    {
        return "Поле «caption» не может быть пустой строкой";
    }
    if (kind.value().empty())
    {
        return "Поле «kind» не может быть пустой строкой";
    }

    return "";
}

bool ItemType::operator==(const ItemType& other) const
{
    return
        id == other.id
        && workflowId == other.workflowId
        && defaultStateId == other.defaultStateId
        && caption == other.caption
        && kind == other.kind
        && defaultContent == other.defaultContent
;
}

} // namespace dto