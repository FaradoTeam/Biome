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
 * @brief Workflow DTO
 */
class Workflow final
{
public:
    using Ptr = std::shared_ptr<Workflow>;
    using ConstPtr = std::shared_ptr<const Workflow>;

public:
    Workflow() = default;
    explicit Workflow(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Workflow& other) const;
    bool operator!=(const Workflow& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Workflow& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Название рабочего процесса
    std::optional<std::string> caption;

    /// Описание рабочего процесса
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const Workflow& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto