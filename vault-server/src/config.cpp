#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "config.h"

#include "common/log/log.h"

bool initConfig(const std::string& configFile)
{
    return CONFIG.load(configFile);
}

Config& Config::instance()
{
    static Config config;
    return config;
}

bool Config::load(const std::string& configFile)
{
    boost::property_tree::ptree configTree;
    try
    {
        boost::property_tree::ini_parser::read_ini(configFile, configTree);
    }
    catch (const boost::property_tree::ini_parser_error& exception)
    {
        LOG_ERROR << "Ошибка чтения файла конфигурации: " << exception.what();
        return false;
    }

    // Секция FileLog

    if (auto value = configTree.get_optional<std::string>("FileLog.logPath"))
        fileLog.logPath = *value;

    if (auto value = configTree.get_optional<unsigned>("FileLog.rotationSize"))
        fileLog.rotationSize = *value;

    if (auto value = configTree.get_optional<bool>("FileLog.needSourceLocation"))
        fileLog.needSourceLocation = *value;

    if (auto value = configTree.get_optional<bool>("FileLog.needThreadId"))
        fileLog.needThreadId = *value;

    if (auto value = configTree.get_optional<std::string>("FileLog.dateTimeFormat"))
        fileLog.dateTimeFormat = *value;

    if (auto value = configTree.get_optional<std::string>("FileLog.severityLevel"))
        fileLog.severityLevel = *value;

    // Секция ConsoleLog

    if (auto value = configTree.get_optional<bool>("ConsoleLog.needSourceLocation"))
        consoleLog.needSourceLocation = *value;

    if (auto value = configTree.get_optional<bool>("ConsoleLog.needThreadId"))
        consoleLog.needThreadId = *value;

    if (auto value = configTree.get_optional<std::string>("ConsoleLog.dateTimeFormat"))
        consoleLog.dateTimeFormat = *value;

    if (auto value = configTree.get_optional<std::string>("ConsoleLog.severityLevel"))
        consoleLog.severityLevel = *value;

    // Секция Database

    if (auto value = configTree.get_optional<std::string>("Database.file"))
        database.file = *value;

    // Секция Network

    if (auto value = configTree.get_optional<std::string>("Network.apiHost"))
        network.apiHost = *value;

    if (auto value = configTree.get_optional<std::uint16_t>("Network.apiPort"))
        network.apiPort = *value;

    LOG_INFO << "Файл конфигурации успешно загружен: " << configFile;

    return true;
}

bool Config::save() const
{
    boost::property_tree::ptree configTree;

    // Секция FileLog

    configTree.put("FileLog.logPath", fileLog.logPath);
    configTree.put("FileLog.rotationSize", fileLog.rotationSize);
    configTree.put("FileLog.needSourceLocation", fileLog.needSourceLocation);
    configTree.put("FileLog.needThreadId", fileLog.needThreadId);
    configTree.put("FileLog.dateTimeFormat", fileLog.dateTimeFormat);
    configTree.put("FileLog.severityLevel", fileLog.severityLevel);

    // Секция ConsoleLog

    configTree.put("ConsoleLog.needSourceLocation", consoleLog.needSourceLocation);
    configTree.put("ConsoleLog.needThreadId", consoleLog.needThreadId);
    configTree.put("ConsoleLog.dateTimeFormat", consoleLog.dateTimeFormat);
    configTree.put("ConsoleLog.severityLevel", consoleLog.severityLevel);

    // Секция Database

    configTree.put("Database.file", database.file);

    // Секция Database

    configTree.put("Network.apiHost", network.apiHost);
    configTree.put("Network.apiPort", network.apiPort);

    try
    {
        boost::property_tree::ini_parser::write_ini(m_configFile, configTree);
    }
    catch (const boost::property_tree::ini_parser_error& exception)
    {
        LOG_ERROR << "Ошибка записи файла конфигурации: " << exception.what();
        return false;
    }
    return true;
}

Config::Config()
{
}
