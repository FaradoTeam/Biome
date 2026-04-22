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
 * @brief Project DTO
 */
class Project final
{
public:
    using Ptr = std::shared_ptr<Project>;
    using ConstPtr = std::shared_ptr<const Project>;

public:
    Project() = default;
    explicit Project(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Project& other) const;
    bool operator!=(const Project& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Project& dto);

public:
    /// Уникальный идентификатор проекта
    std::optional<int64_t> id;

    /// Идентификатор родительского проекта (null для корневых)
    std::optional<int64_t> parentId;

    /// Название проекта
    std::optional<std::string> caption;

    /// Описание проекта
    std::optional<std::string> description;

    /// Дата и время создания
    std::optional<std::chrono::system_clock::time_point> createdAt;

    /// Дата и время последнего обновления
    std::optional<std::chrono::system_clock::time_point> updatedAt;

    /// Флаг архивации (мягкое удаление)
    std::optional<bool> isArchive;

};

inline std::ostream& operator<<(std::ostream& os, const Project& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto