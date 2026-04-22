#pragma once

#include <optional>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "common/helpers/print_helpers.h"


namespace dto
{

/**
 * @brief PageResponse DTO
 */
class PageResponse final
{
public:
    using Ptr = std::shared_ptr<PageResponse>;
    using ConstPtr = std::shared_ptr<const PageResponse>;

public:
    PageResponse() = default;
    explicit PageResponse(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const PageResponse& other) const;
    bool operator!=(const PageResponse& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const PageResponse& dto);

public:
    std::optional<std::vector<std::string>> items;

    std::optional<int64_t> totalCount;

    std::optional<int64_t> page;

    std::optional<int64_t> pageSize;

};

inline std::ostream& operator<<(std::ostream& os, const PageResponse& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto