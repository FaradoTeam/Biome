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
 * @brief ItemDocument DTO
 */
class ItemDocument final
{
public:
    using Ptr = std::shared_ptr<ItemDocument>;
    using ConstPtr = std::shared_ptr<const ItemDocument>;

public:
    ItemDocument() = default;
    explicit ItemDocument(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const ItemDocument& other) const;
    bool operator!=(const ItemDocument& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const ItemDocument& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Идентификатор элемента
    std::optional<int64_t> itemId;

    /// Идентификатор документа
    std::optional<int64_t> documentId;

};

inline std::ostream& operator<<(std::ostream& os, const ItemDocument& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto