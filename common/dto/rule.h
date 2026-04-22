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
 * @brief Rule DTO
 */
class Rule final
{
public:
    using Ptr = std::shared_ptr<Rule>;
    using ConstPtr = std::shared_ptr<const Rule>;

public:
    Rule() = default;
    explicit Rule(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Rule& other) const;
    bool operator!=(const Rule& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Rule& dto);

public:
    /// Уникальный идентификатор правила
    std::optional<int64_t> id;

    /// Идентификатор роли
    std::optional<int64_t> roleId;

    /// Право на создание корневых проектов
    std::optional<bool> isRootProjectCreator;

};

inline std::ostream& operator<<(std::ostream& os, const Rule& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto