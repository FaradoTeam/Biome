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
 * @brief RuleProject DTO
 */
class RuleProject final
{
public:
    using Ptr = std::shared_ptr<RuleProject>;
    using ConstPtr = std::shared_ptr<const RuleProject>;

public:
    RuleProject() = default;
    explicit RuleProject(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const RuleProject& other) const;
    bool operator!=(const RuleProject& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const RuleProject& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор правила
    std::optional<int64_t> ruleId;

    /// Идентификатор проекта
    std::optional<int64_t> projectId;

    /// Право на просмотр элементов (items) проекта
    std::optional<bool> isReader;

    /// Право на создание и изменение элементов (items) проекта
    std::optional<bool> isWriter;

    /// Право на создание и изменение подпроектов
    std::optional<bool> isProjectEditor;

    /// Право на создание и изменение фаз
    std::optional<bool> isPhaseEditor;

    /// Право на создание и изменение досок
    std::optional<bool> isBoardEditor;

};

inline std::ostream& operator<<(std::ostream& os, const RuleProject& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto