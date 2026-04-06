#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#ifdef _WIN32
#define strptime _strptime
#endif

namespace db
{

/**
 * @brief Тип для хранения бинарных данных (BLOB).
 */
using Blob = std::vector<uint8_t>;

/**
 * @brief Тип для хранения даты/времени (момент времени в UTC).
 */
using DateTime = std::chrono::system_clock::time_point;

/**
 * @brief Универсальное поле результата запроса.
 *
 * Может хранить NULL, целое число, вещественное число, строку,
 * BLOB или дату/время. Поддерживает type-safe доступ.
 */
struct FieldValue final
{
public:
    enum class Type
    {
        Null,
        Int64,
        Double,
        String,
        Blob,
        DateTime
    };

public:
    FieldValue() = default;
    FieldValue(std::nullptr_t)
        : type(Type::Null)
    {
    }

    explicit FieldValue(int64_t v)
        : type(Type::Int64)
        , value(v)
    {
    }
    explicit FieldValue(double v)
        : type(Type::Double)
        , value(v)
    {
    }

    FieldValue(const std::string& v)
        : type(Type::String)
        , value(v)
    {
    }
    FieldValue(std::string&& v)
        : type(Type::String)
        , value(std::move(v))
    {
    }
    FieldValue(const char* v)
        : type(Type::String)
        , value(std::string(v))
    {
    }

    FieldValue(const Blob& v)
        : type(Type::Blob)
        , value(v)
    {
    }
    FieldValue(Blob&& v)
        : type(Type::Blob)
        , value(std::move(v))
    {
    }

    FieldValue(const DateTime& v)
        : type(Type::DateTime)
        , value(v)
    {
    }
    FieldValue(DateTime&& v)
        : type(Type::DateTime)
        , value(std::move(v))
    {
    }

public:
    bool isNull() const
    {
        return (type == Type::Null);
    }

    int64_t asInt64() const
    {
        if (type != Type::Int64)
            throw std::runtime_error("Not an Int64");
        return std::get<int64_t>(value);
    }

    double asDouble() const
    {
        if (type != Type::Double)
            throw std::runtime_error("Not a Double");
        return std::get<double>(value);
    }

    const std::string& asString() const
    {
        if (type != Type::String)
            throw std::runtime_error("Not a String");
        return std::get<std::string>(value);
    }

    const Blob& asBlob() const
    {
        if (type != Type::Blob)
            throw std::runtime_error("Not a Blob");
        return std::get<Blob>(value);
    }

    DateTime asDateTime() const
    {
        if (type != Type::DateTime)
            throw std::runtime_error("Not a DateTime");
        return std::get<DateTime>(value);
    }

public:
    Type type = Type::Null;
    std::variant<std::monostate, int64_t, double, std::string, Blob, DateTime> value;
};

/**
 * @brief Конфигурация базы данных (пары ключ-значение).
 *
 * Пример для SQLite: { {"database", "test.db"} }
 */
using DatabaseConfig = std::unordered_map<std::string, std::string>;

/// Вспомогательная функция для преобразования даты и времени в строку (ISO8601).
inline std::string dateTimeToString(const DateTime& dt)
{
    auto tt = std::chrono::system_clock::to_time_t(dt);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&tt));
    return buf;
}

/// Вспомогательная функция для преобразования строки в DateTime.
inline DateTime stringToDateTime(const std::string& str)
{
    std::tm tm = {};
    strptime(str.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

} // namespace db
