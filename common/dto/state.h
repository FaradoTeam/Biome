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
 * @brief State DTO
 */
class State final
{
public:
    using Ptr = std::shared_ptr<State>;
    using ConstPtr = std::shared_ptr<const State>;

public:
    State() = default;
    explicit State(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const State& other) const;
    bool operator!=(const State& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const State& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор рабочего процесса
    std::optional<int64_t> workflowId;

    /// Название состояния
    std::optional<std::string> caption;

    /// Описание состояния
    std::optional<std::string> description;

    /// Вес состояния (для метрик)
    std::optional<int64_t> weight;

    /// Порядковый номер для сортировки
    std::optional<int64_t> orderNumber;

    /// Флаг архивного состояния (редко отображается)
    std::optional<bool> isArchive;

    /// Флаг очереди (для определения активных работ)
    std::optional<bool> isQueue;

};

inline std::ostream& operator<<(std::ostream& os, const State& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto