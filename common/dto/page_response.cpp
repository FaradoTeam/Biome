#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "common/helpers/time_helpers.h"

#include "page_response.h"

namespace dto
{

PageResponse::PageResponse(const nlohmann::json& json)
{
    fromJson(json);
}

nlohmann::json PageResponse::toJson() const
{
    nlohmann::json result;

    // items
    if (items.has_value())
    {
        if (!items.value().empty())
        {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& item : items.value())
            {
                arr.push_back(item);
            }
            result["items"] = arr;
        }
    }
    // totalCount
    if (totalCount.has_value())
    {
        result["total_count"] = totalCount.value();
    }
    // page
    if (page.has_value())
    {
        result["page"] = page.value();
    }
    // pageSize
    if (pageSize.has_value())
    {
        result["page_size"] = pageSize.value();
    }

    return result;
}

bool PageResponse::fromJson(const nlohmann::json& json)
{
    bool success = true;

    // items
    if (json.contains("items") && !json["items"].is_null())
    {
        try
        {
            std::vector<std::string> vec;
            for (const auto& item : json["items"])
            {
                vec.push_back(item.get<std::string>());
            }
            items = std::move(vec);
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        items = std::nullopt;
    }
    // totalCount
    if (json.contains("total_count") && !json["total_count"].is_null())
    {
        try
        {
            totalCount = json["total_count"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        totalCount = std::nullopt;
    }
    // page
    if (json.contains("page") && !json["page"].is_null())
    {
        try
        {
            page = json["page"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        page = std::nullopt;
    }
    // pageSize
    if (json.contains("page_size") && !json["page_size"].is_null())
    {
        try
        {
            pageSize = json["page_size"].get<int64_t>();
        }
        catch (const std::exception& e)
        {
            success = false;
        }
    }
    else
    {
        pageSize = std::nullopt;
    }

    return success;
}

bool PageResponse::isValid() const
{

    // Дополнительные проверки для непустых значений

    return true;
}

std::string PageResponse::validationError() const
{


    return "";
}

bool PageResponse::operator==(const PageResponse& other) const
{
    return
        items == other.items
        && totalCount == other.totalCount
        && page == other.page
        && pageSize == other.pageSize
;
}

} // namespace dto