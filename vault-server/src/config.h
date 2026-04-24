#pragma once

#include <cstdint>
#include <string>

#define CONFIG Config::instance()

bool initConfig(const std::string& configFile);

/// Общая конфигурация системы.
class Config final
{
public:
    static Config& instance();

public:
    bool load(const std::string& configFile);
    bool save() const;

public:
    /// Файловый лог.
    struct FileLog final
    {
        /// Путь к папке с логами.
        std::string logPath = "./logs/";

        /// Параметр отвечает за создание нового файла лога, когда текущий
        /// достигает заданного размера. Значение по умолчанию 10 МБ.
        unsigned rotationSize = 10 * 1024 * 1024;

        /// Флаг отвечает за вывод имени файла исходного, номера строки и имени
        /// функции в исходном коде.
        bool needSourceLocation = true;

        /// Флаг отвечает за вывод идентификатора потока в лог файл.
        bool needThreadId = true;

        /// Параметр отвечает за формат вывода даты и времени.
        std::string dateTimeFormat = "%Y-%m-%d %H:%M:%S.%f";

        /// Минимальный уровень важности фиксируемых сообщений.
        std::string severityLevel = "info";
    };

    struct ConsoleLog final
    {
        /// Флаг отвечает за вывод имени файла исходного, номера строки и имени
        /// функции в исходном коде.
        bool needSourceLocation = false;

        /// Флаг отвечает за вывод идентификатора потока в лог.
        bool needThreadId = true;

        /// Параметр отвечает за формат вывода даты и времени.
        std::string dateTimeFormat = "%Y-%m-%d %H:%M:%S.%f";

        /// Минимальный уровень важности фиксируемых сообщений.
        std::string severityLevel = "info";
    };

    struct Database final
    {
        /// Имя файла базы данных.
        std::string file = "database.sqlite";
    };

    struct Network final
    {
        /// Имя подключения к службе предоставления данных.
        std::string apiHost = "http://127.0.0.1";

        /// Порт подключения к службе предоставления данных.
        std::uint16_t apiPort = 8090;
    };

    FileLog fileLog;
    ConsoleLog consoleLog;
    Database database;
    Network network;

private:
    Config();

private:
    std::string m_configFile;
};
