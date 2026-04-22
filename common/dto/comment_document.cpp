#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "comment_document.h"

namespace dto
{

CommentDocument::CommentDocument(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json CommentDocument::toJson() const
{
    nlohmann::json result;

    // Уникальный идентификатор
    if (id.has_value())
    {
        result["id"] = id.value();
    }
    // Идентификатор комментария
    if (commentId.has_value())
    {
        result["comment_id"] = commentId.value();
    }
    // Идентификатор документа
    if (documentId.has_value())
    {
        result["document_id"] = documentId.value();
    }

    return result;
}

bool CommentDocument::fromJson(const nlohmann::json& json)
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
    // Идентификатор комментария
    if (json.contains("comment_id") && !json["comment_id"].is_null())
    {
        try
        {
            commentId = json["comment_id"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        commentId = std::nullopt;
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

bool CommentDocument::isValid() const
{
    if (!commentId.has_value())
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

std::string CommentDocument::validationError() const
{
    if (!commentId.has_value())
    {
        return "Поле «comment_id» является обязательным для заполнения";
    }
    if (!documentId.has_value())
    {
        return "Поле «document_id» является обязательным для заполнения";
    }


    return "";
}

bool CommentDocument::operator==(const CommentDocument& other) const
{
    return
        id == other.id
        && commentId == other.commentId
        && documentId == other.documentId
;
}

} // namespace dto