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
 * @brief RuleItemType DTO
 */
class RuleItemType final
{
public:
    using Ptr = std::shared_ptr<RuleItemType>;
    using ConstPtr = std::shared_ptr<const RuleItemType>;

public:
    RuleItemType() = default;
    explicit RuleItemType(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const RuleItemType& other) const;
    bool operator!=(const RuleItemType& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const RuleItemType& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор правила
    std::optional<int64_t> ruleId;

    /// Идентификатор типа элемента
    std::optional<int64_t> itemTypeId;

    /// Право на чтение элементов данного типа
    std::optional<bool> isReader;

    /// Право на создание и изменение элементов данного типа
    std::optional<bool> isWriter;

};

inline std::ostream& operator<<(std::ostream& os, const RuleItemType& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto