#include <atomic>
#include <csignal>
#include <iostream>

#include <boost/program_options.hpp>

#include "common/log/log.h"

#include "config.h"
#include "application.h"

std::atomic<server::Application*> applicationPtr { nullptr };

void signalHandler(int signal)
{
    LOG_INFO << "Получен сигнал " << signal;
    if (auto app = applicationPtr.load())
        app->stop();
}

void printHelp(const boost::program_options::options_description& options)
{
    std::cout
        << "Использование: biome-vault-server [ПАРАМЕТР]…" << std::endl
        << "Предоставляет доступ к данным системы управления проектами «Биом»."
        << std::endl << std::endl;
    std::cout << options << std::endl;
}

std::string configFilename(
    const boost::program_options::variables_map& variables
)
{
    if (variables.count("config"))
        return variables["config"].as<std::string>();

    return "config.ini";
}

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description options("Допустимые параметры");
    options.add_options()(
        "help,h",
        "вывести справочное сообщение с перечнем разрешенных параметров"
    )(
        "config,c",
        po::value<std::string>(),
        "задать альтернативный путь к файлу конфигурации"
    );

    po::variables_map variables;
    try
    {
        po::store(po::parse_command_line(argc, argv, options), variables);
        po::notify(variables);
    }
    catch (const po::error& exception)
    {
        LOG_ERROR << exception.what();
        printHelp(options);
        return EXIT_FAILURE;
    }

    if (variables.count("help"))
    {
        printHelp(options);
        return EXIT_SUCCESS;
    }

    initConfig(configFilename(variables));
    initLog(
        "bvs",
        CONFIG.fileLog.logPath,
        CONFIG.fileLog.rotationSize,
        CONFIG.fileLog.needSourceLocation,
        CONFIG.fileLog.needThreadId,
        CONFIG.fileLog.dateTimeFormat,
        CONFIG.fileLog.severityLevel,
        CONFIG.consoleLog.needSourceLocation,
        CONFIG.consoleLog.needThreadId,
        CONFIG.consoleLog.dateTimeFormat,
        CONFIG.consoleLog.severityLevel
    );

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    LOG_INFO << "Запуск сервера «Биом»...";

    int result = 0;
    {
        server::Application application;
        applicationPtr.store(&application);
        result = application.run();
    }

    LOG_INFO << "Сервер «Биом» был остановлен. Код: " << result;
    return result;
}
