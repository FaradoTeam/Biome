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
 * @brief Team DTO
 */
class Team final
{
public:
    using Ptr = std::shared_ptr<Team>;
    using ConstPtr = std::shared_ptr<const Team>;

public:
    Team() = default;
    explicit Team(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Team& other) const;
    bool operator!=(const Team& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Team& dto);

public:
    /// Уникальный идентификатор команды
    std::optional<int64_t> id;

    /// Название команды
    std::optional<std::string> caption;

    /// Описание команды
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const Team& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto