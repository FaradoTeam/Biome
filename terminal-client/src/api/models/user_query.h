#pragma once

#include <optional>
#include <string>

namespace terminal::models
{

/**
 * @brief Параметры запроса списка пользователей.
 */
struct UserQuery final
{
    int page { 1 };
    int pageSize { 20 };

    std::string login;   ///< Фильтр по логину (частичное совпадение)
    std::string name;    ///< Фильтр по ФИО   (частичное совпадение)
    std::string email;   ///< Фильтр по email (частичное совпадение)

    /// Фильтр по блокировке: nullopt — все, true — только заблокированные, false — только активные.
    std::optional<bool> isBlocked;
};

} // namespace terminal::models
