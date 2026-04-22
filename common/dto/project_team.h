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
 * @brief ProjectTeam DTO
 */
class ProjectTeam final
{
public:
    using Ptr = std::shared_ptr<ProjectTeam>;
    using ConstPtr = std::shared_ptr<const ProjectTeam>;

public:
    ProjectTeam() = default;
    explicit ProjectTeam(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ProjectTeam& other) const;
    bool operator!=(const ProjectTeam& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ProjectTeam& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор проекта
    std::optional<int64_t> projectId;

    /// Идентификатор команды
    std::optional<int64_t> teamId;

};

inline std::ostream& operator<<(std::ostream& os, const ProjectTeam& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto