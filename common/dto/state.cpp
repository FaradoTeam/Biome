#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "state.h"

namespace dto
{

State::State(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json State::toJson() const
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
    // Название состояния
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание состояния
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Вес состояния (для метрик)
    if (weight.has_value())
    {
        result["weight"] = weight.value();
    }
    // Порядковый номер для сортировки
    if (orderNumber.has_value())
    {
        result["order_number"] = orderNumber.value();
    }
    // Флаг архивного состояния (редко отображается)
    if (isArchive.has_value())
    {
        result["is_archive"] = isArchive.value();
    }
    // Флаг очереди (для определения активных работ)
    if (isQueue.has_value())
    {
        result["is_queue"] = isQueue.value();
    }

    return result;
}

bool State::fromJson(const nlohmann::json& json)
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
    // Название состояния
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
    // Описание состояния
    if (json.contains("description") && !json["description"].is_null())
    {
        try
        {
            description = json["description"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        description = std::nullopt;
    }
    // Вес состояния (для метрик)
    if (json.contains("weight") && !json["weight"].is_null())
    {
        try
        {
            weight = json["weight"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        weight = std::nullopt;
    }
    // Порядковый номер для сортировки
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
    // Флаг архивного состояния (редко отображается)
    if (json.contains("is_archive") && !json["is_archive"].is_null())
    {
        try
        {
            isArchive = json["is_archive"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isArchive = std::nullopt;
    }
    // Флаг очереди (для определения активных работ)
    if (json.contains("is_queue") && !json["is_queue"].is_null())
    {
        try
        {
            isQueue = json["is_queue"].get<bool>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        isQueue = std::nullopt;
    }

    return success;
}

bool State::isValid() const
{
    if (!workflowId.has_value())
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

std::string State::validationError() const
{
    if (!workflowId.has_value())
    {
        return "Поле «workflow_id» является обязательным для заполнения";
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

bool State::operator==(const State& other) const
{
    return
        id == other.id
        && workflowId == other.workflowId
        && caption == other.caption
        && description == other.description
        && weight == other.weight
        && orderNumber == other.orderNumber
        && isArchive == other.isArchive
        && isQueue == other.isQueue
;
}

} // namespace dto