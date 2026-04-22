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
 * @brief Document DTO
 */
class Document final
{
public:
    using Ptr = std::shared_ptr<Document>;
    using ConstPtr = std::shared_ptr<const Document>;

public:
    Document() = default;
    explicit Document(const nlohmann::json& json);

    // Сериализация
    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json& json);

    // Валидация
    bool isValid() const;
    std::string validationError() const;

    // Сравнение
    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const
    {
        return !(*this == other);
    }

    // Потоковый вывод для отладки
    friend std::ostream& operator<<(std::ostream& os, const Document& dto);

public:
    /// Уникальный идентификатор
    std::optional<int64_t> id;

    /// Название документа
    std::optional<std::string> caption;

    /// Описание документа
    std::optional<std::string> description;

    /// Путь к файлу на диске
    std::optional<std::string> path;

    /// Исходное имя файла
    std::optional<std::string> filename;

    /// Размер файла в байтах
    std::optional<int64_t> size;

    /// MIME-тип файла
    std::optional<std::string> mimeType;

    /// Время загрузки
    std::optional<std::chrono::system_clock::time_point> uploadedAt;

    /// Идентификатор пользователя
    std::optional<int64_t> uploadedByUserId;

};

inline std::ostream& operator<<(std::ostream& os, const Document& dto)
{
    os << dto.toJson().dump(2);
    return os;
}

} // namespace dto