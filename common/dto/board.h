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
 * @brief Board DTO
 */
class Board final
{
public:
    using Ptr = std::shared_ptr<Board>;
    using ConstPtr = std::shared_ptr<const Board>;

public:
    Board() = default;
    explicit Board(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Board& other) const;
    bool operator!=(const Board& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Board& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор рабочего процесса
    std::optional<int64_t> workflowId;

    /// Идентификатор проекта
    std::optional<int64_t> projectId;

    /// Идентификатор фазы
    std::optional<int64_t> phaseId;

    /// Название доски
    std::optional<std::string> caption;

    /// Описание доски
    std::optional<std::string> description;

};

inline std::ostream& operator<<(std::ostream& os, const Board& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto