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
 * @brief ActivatePlanRequest DTO
 */
class ActivatePlanRequest final
{
public:
    using Ptr = std::shared_ptr<ActivatePlanRequest>;
    using ConstPtr = std::shared_ptr<const ActivatePlanRequest>;

public:
    ActivatePlanRequest() = default;
    explicit ActivatePlanRequest(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ActivatePlanRequest& other) const;
    bool operator!=(const ActivatePlanRequest& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ActivatePlanRequest& dto);

public:
    /// Кто активирует план
    std::optional<int64_t> activatedByUserId;

};

inline std::ostream& operator<<(std::ostream& os, const ActivatePlanRequest& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto