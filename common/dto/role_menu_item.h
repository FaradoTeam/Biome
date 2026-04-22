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
 * @brief RoleMenuItem DTO
 */
class RoleMenuItem final
{
public:
    using Ptr = std::shared_ptr<RoleMenuItem>;
    using ConstPtr = std::shared_ptr<const RoleMenuItem>;

public:
    RoleMenuItem() = default;
    explicit RoleMenuItem(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const RoleMenuItem& other) const;
    bool operator!=(const RoleMenuItem& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const RoleMenuItem& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор роли
    std::optional<int64_t> roleId;

    /// Название пункта меню
    std::optional<std::string> caption;

    /// Ссылка (URL)
    std::optional<std::string> link;

    /// Значок пункта меню
    std::optional<std::string> icon;

};

inline std::ostream& operator<<(std::ostream& os, const RoleMenuItem& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto