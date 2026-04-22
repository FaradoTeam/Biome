#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "document.h"

namespace dto
{

Document::Document(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json Document::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Название документа
    if (caption.has_value())
    {
        result["caption"] = caption.value();
    }
    // Описание документа
    if (description.has_value())
    {
        result["description"] = description.value();
    }
    // Путь к файлу на диске
    if (path.has_value())
    {
        result["path"] = path.value();
    }
    // Исходное имя файла
    if (filename.has_value())
    {
        result["filename"] = filename.value();
    }
    // Размер файла в байтах
    if (size.has_value())
    {
        result["size"] = size.value();
    }
    // MIME-тип файла
    if (mimeType.has_value())
    {
        result["mime_type"] = mimeType.value();
    }
    // Время загрузки
    if (uploadedAt.has_value())
    {
        result["uploaded_at"] = timePointToSeconds(uploadedAt.value());
    }
    // Идентификатор пользователя
    if (uploadedByUserId.has_value())
    {
        result["uploaded_by_user_id"] = uploadedByUserId.value();
    }

    return result;
}

bool Document::fromJson(const nlohmann::json& json)
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
    // Название документа
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
    // Описание документа
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
    // Путь к файлу на диске
    if (json.contains("path") && !json["path"].is_null())
    {
        try
        {
            path = json["path"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        path = std::nullopt;
    }
    // Исходное имя файла
    if (json.contains("filename") && !json["filename"].is_null())
    {
        try
        {
            filename = json["filename"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        filename = std::nullopt;
    }
    // Размер файла в байтах
    if (json.contains("size") && !json["size"].is_null())
    {
        try
        {
            size = json["size"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        size = std::nullopt;
    }
    // MIME-тип файла
    if (json.contains("mime_type") && !json["mime_type"].is_null())
    {
        try
        {
            mimeType = json["mime_type"].get<std::string>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        mimeType = std::nullopt;
    }
    // Время загрузки
    if (json.contains("uploaded_at") && !json["uploaded_at"].is_null())
    {
        try
        {
            auto timestampValue = json["uploaded_at"].get<int64_t>();
            uploadedAt = secondsToTimePoint(timestampValue);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        uploadedAt = std::nullopt;
    }
    // Идентификатор пользователя
    if (json.contains("uploaded_by_user_id") && !json["uploaded_by_user_id"].is_null())
    {
        try
        {
            uploadedByUserId = json["uploaded_by_user_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        uploadedByUserId = std::nullopt;
    }

    return success;
}

bool Document::isValid() const
{
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

std::string Document::validationError() const
{
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

bool Document::operator==(const Document& other) const
{
    return
        id == other.id
        && caption == other.caption
        && description == other.description
        && path == other.path
        && filename == other.filename
        && size == other.size
        && mimeType == other.mimeType
        && uploadedAt == other.uploadedAt
        && uploadedByUserId == other.uploadedByUserId
;
}

} // namespace dto