#pragma once

#include <cpprest/json.h>
#include <nlohmann/json.hpp>

namespace dto
{

// FIXME : Отказаться от таких преобразований в пользу более удачного архитектурного решения.
// Отказаться от сериализации DTO в пользу рефлексии (glaze или gcc16 c C++26).
inline web::json::value toWebJson(const nlohmann::json& json)
{
    return web::json::value::parse(
        utility::conversions::to_string_t(
            json.dump()
        )
    );
}

inline nlohmann::json toNlohmannJson(const web::json::value& value)
{
    return nlohmann::json::parse(
        utility::conversions::to_utf8string(
            value.serialize()
        )
    );
}

} // namespace dto
