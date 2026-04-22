#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "item_document.h"

namespace dto
{

ItemDocument::ItemDocument(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json ItemDocument::toJson() const
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
    // Идентификатор документа
    if (documentId.has_value())
    {
        result["document_id"] = documentId.value();
    }

    return result;
}

bool ItemDocument::fromJson(const nlohmann::json& json)
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
    // Идентификатор документа
    if (json.contains("document_id") && !json["document_id"].is_null())
    {
        try
        {
            documentId = json["document_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        documentId = std::nullopt;
    }

    return success;
}

bool ItemDocument::isValid() const
{
    if (!itemId.has_value())
    {
        return false;
    }
    if (!documentId.has_value())
    {
        return false;
    }

    // Дополнительные проверки для непустых значений

    return true;
}

std::string ItemDocument::validationError() const
{
    if (!itemId.has_value())
    {
        return "Поле «item_id» является обязательным для заполнения";
    }
    if (!documentId.has_value())
    {
        return "Поле «document_id» является обязательным для заполнения";
    }


    return "";
}

bool ItemDocument::operator==(const ItemDocument& other) const
{
    return
        id == other.id
        && itemId == other.itemId
        && documentId == other.documentId
;
}

} // namespace dto