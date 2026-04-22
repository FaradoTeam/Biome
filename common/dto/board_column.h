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
 * @brief BoardColumn DTO
 */
class BoardColumn final
{
public:
    using Ptr = std::shared_ptr<BoardColumn>;
    using ConstPtr = std::shared_ptr<const BoardColumn>;

public:
    BoardColumn() = default;
    explicit BoardColumn(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const BoardColumn& other) const;
    bool operator!=(const BoardColumn& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const BoardColumn& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор доски
    std::optional<int64_t> boardId;

    /// Идентификатор состояния
    std::optional<int64_t> stateId;

    /// Порядковый номер колонки на доске
    std::optional<int64_t> orderNumber;

    /// JSON с настройками (WIP limit
    std::optional<std::string> settings;

};

inline std::ostream& operator<<(std::ostream& os, const BoardColumn& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto