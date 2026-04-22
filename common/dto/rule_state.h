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
 * @brief RuleState DTO
 */
class RuleState final
{
public:
    using Ptr = std::shared_ptr<RuleState>;
    using ConstPtr = std::shared_ptr<const RuleState>;

public:
    RuleState() = default;
    explicit RuleState(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const RuleState& other) const;
    bool operator!=(const RuleState& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const RuleState& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор правила
    std::optional<int64_t> ruleId;

    /// Идентификатор целевого состояния
    std::optional<int64_t> stateId;

    /// Разрешение на перевод элемента в это состояние
    std::optional<bool> isStateAllowed;

};

inline std::ostream& operator<<(std::ostream& os, const RuleState& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto