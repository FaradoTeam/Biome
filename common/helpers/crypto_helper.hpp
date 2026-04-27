#pragma once

#include <concepts>
#include <memory>
#include <ranges>
#include <span>
#include <string>

#include <openssl/evp.h>

namespace crypto
{

/**
 * @brief Класс-обёртка для управления контекстом дайджеста OpenSSL (EVP_MD_CTX).
 *
 * Инкапсулирует создание, использование и автоматическое освобождение ресурсов
 * контекста с помощью умного указателя с пользовательским удалителем.
 * Гарантирует безопасное управление памятью при работе с EVP API.
 */
class EvpContext
{
public:
    EvpContext()
        : m_context(EVP_MD_CTX_new())
    {
        if (!m_context)
            throw std::runtime_error("EVP_MD_CTX_new failed");
    }

    EVP_MD_CTX* get()
    {
        return m_context.get();
    }

    /**
     * @brief Инициализирует контекст для работы с указанным алгоритмом хеширования.
     *
     * @param md Указатель на структуру алгоритма (например, EVP_sha256()).
     * @throw std::runtime_error если инициализация не удалась.
     */
    void init(const EVP_MD* md)
    {
        if (!EVP_DigestInit_ex(get(), md, nullptr))
            throw std::runtime_error("EVP_DigestInit_ex failed");
    }

    /**
     * @brief Добавляет порцию данных для последующего вычисления хеша.
     *
     * Позволяет подавать данные частями (потоковая обработка).
     *
     * @param data Ссылка на span с байтами данных.
     * @throw std::runtime_error если обновление контекста не удалось.
     */
    void update(std::span<const unsigned char> data)
    {
        if (!EVP_DigestUpdate(get(), data.data(), data.size()))
            throw std::runtime_error("EVP_DigestUpdate failed");
    }

    /**
     * @brief Перегрузка для работы со строками.
     */
    void update(const std::string& str)
    {
        update(
            std::span<const unsigned char>(
                reinterpret_cast<const unsigned char*>(str.data()),
                str.size()
            )
        );
    }

    /**
     * @brief Завершает вычисление хеша и возвращает его бинарное представление.
     *
     * После вызова этого метода контекст становится невалидным для дальнейшего
     * использования (финализирован).
     *
     * @return std::vector<unsigned char> Вектор, содержащий бинарный хеш.
     * @throw std::runtime_error если финализация не удалась.
     */
    std::vector<unsigned char> finalize()
    {
        std::vector<unsigned char> hash(EVP_MAX_MD_SIZE);
        unsigned int len = 0;
        if (!EVP_DigestFinal_ex(get(), hash.data(), &len))
            throw std::runtime_error("EVP_DigestFinal_ex failed");
        hash.resize(len);
        return hash;
    }

private:
    struct Deleter
    {
        void operator()(EVP_MD_CTX* ctx)
        {
            EVP_MD_CTX_free(ctx);
        }
    };
    std::unique_ptr<EVP_MD_CTX, Deleter> m_context;
};

/**
 * @brief Преобразует бинарные данные в шестнадцатеричную строку.
 *
 * Каждый байт представляется двумя символами нижнего регистра (00–ff).
 * Результирующая строка будет ровно в два раза длиннее входных данных.
 *
 * @param data Ссылка на span с байтами для преобразования.
 * @return std::string Шестнадцатеричное представление данных.
 */
inline std::string toHex(std::span<const unsigned char> data)
{
    // Лямбда для преобразования одного полубайта в hex-символ
    auto toHexChar = [](unsigned char byte, bool high) -> char
    {
        constexpr char hex[] = "0123456789abcdef";
        // high == true  → берём старшие 4 бита (сдвиг вправо на 4)
        // high == false → берём младшие 4 бита (маска 0x0F)
        return hex[high ? (byte >> 4) : (byte & 0x0F)];
    };

    std::string result;
    result.reserve(data.size() * 2);

    for (auto byte : data)
    {
        result += toHexChar(byte, true); // Старший полубайт
        result += toHexChar(byte, false); // Младший полубайт
    }

    return result;
}

/**
 * @brief Вычисляет хеш строки с использованием заданного алгоритма.
 *
 * Связывает воедино создание контекста, инициализацию, передачу данных
 * и финализацию в одну функцию.
 *
 * @param input Строка для хеширования.
 * @param algorithm Указатель на структуру алгоритма OpenSSL.
 * @return std::string Шестнадцатеричная строка-хеш.
 */
inline std::string computeHash(
    const std::string& input,
    const EVP_MD* algorithm
)
{
    EvpContext ctx;
    ctx.init(algorithm);
    ctx.update(input);
    auto hash = ctx.finalize();
    return toHex(hash);
}

/**
 * @brief Вычисляет SHA-256 хеш строки.
 *
 * @param input Строка для хеширования.
 * @return std::string SHA-256 хеш в шестнадцатеричном виде.
 */
inline std::string sha256(const std::string& input)
{
    return computeHash(input, EVP_sha256());
}

/**
 * @brief Вычисляет SHA-512 хеш строки.
 *
 * @param input Строка для хеширования.
 * @return std::string SHA-512 хеш в шестнадцатеричном виде.
 */
inline std::string sha512(const std::string& input)
{
    return computeHash(input, EVP_sha512());
}

} // namespace crypto
