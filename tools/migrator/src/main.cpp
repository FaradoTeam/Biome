#include <iostream>
#include <memory>
#include <stdexcept>

#include <boost/program_options.hpp>

#include "common/log/log.h"

#include "database_factory.h"

#include "migration_manager.h"

#include "migrations_sqlite/v1_initial_schema.h"
#include "migrations_sqlite/v2_default_data.h"

namespace po = boost::program_options;

/**
 * @brief Регистрирует все доступные миграции.
 * @param manager Менеджер миграций
 */
void registerMigrations(db::MigrationManager& manager)
{
    // Регистрируем миграцию v1
    manager.registerMigration(std::make_unique<db::migrations::V1_InitialSchema>());

    // Регистрируем миграцию v2 - дефолтные данные
    manager.registerMigration(std::make_unique<db::migrations::V2_DefaultData>());
}

/**
 * @brief Выводит список доступных миграций.
 * @param manager Менеджер миграций
 */
void listMigrations(const db::MigrationManager& manager)
{
    const auto migrations = manager.availableMigrations();
    const unsigned int currentVersion = manager.currentVersion();

    LOG_INFO << "Доступные версии миграции:";
    LOG_INFO << "  Текущая версия: " << currentVersion;

    for (const auto& [version, description] : migrations)
    {
        const std::string marker = (version == currentVersion) ? "[*] " : "[ ] ";
        LOG_INFO
            << "  " << marker << "v" << version << ": " << description;
    }
}

/**
 * @brief Показывает текущую версию базы данных.
 * @param manager Менеджер миграций
 */
void showCurrentVersion(const db::MigrationManager& manager)
{
    const unsigned int version = manager.currentVersion();
    LOG_INFO << "Текущая версия базы данных: " << version;
}

void printHelp(const boost::program_options::options_description& options)
{
    std::cout
        << "Использование: migrator [ПАРАМЕТР]…" << std::endl
        << "Утилита для пошагового обновления и отката схемы базы данных."
        << std::endl
        << std::endl;
    std::cout << options << std::endl;
    std::cout
        << std::endl
        << "Примеры:" << std::endl
        << "  ./migrator --database=test.db --current" << std::endl
        << "  ./migrator --database=test.db --list" << std::endl
        << "  ./migrator --database=test.db --upgrade-all" << std::endl
        << "  ./migrator --database=test.db --upgrade" << std::endl
        << "  ./migrator --database=test.db --downgrade" << std::endl;
}

int main(int argc, char* argv[])
{
    initLog("migrator");

    try
    {
        // Настройка параметров командной строки
        po::options_description options("Допустимые параметры");
        options.add_options()(
            "help,h",
            "вывести справочное сообщение с перечнем разрешенных параметров"
        )(
            "database,s",
            po::value<std::string>(),
            "путь к базе данных (пример: sqlite://путь/к/файлу.db)"
        )(
            "current,c",
            "показать текущую версию базы данных"
        )(
            "list,l",
            "список доступных миграций"
        )(
            "upgrade-all,a",
            "обновить базу данных до последней версии"
        )(
            "upgrade,u",
            "обновить базу данных на одну версию"
        )(
            "downgrade,d",
            "понизить версию базы данных на одну версию"
        );

        po::variables_map variables;
        try
        {
            po::store(po::parse_command_line(argc, argv, options), variables);
            po::notify(variables);
        }
        catch (const po::error& exception)
        {
            LOG_ERROR << "Ошибка: " << exception.what();
            printHelp(options);
            return EXIT_FAILURE;
        }

        if (variables.count("help") || argc == 1)
        {
            printHelp(options);
            return EXIT_SUCCESS;
        }

        // Проверяем обязательные параметры
        if (!variables.count("database"))
        {
            LOG_ERROR << "Ошибка: --database обязательный аргумент!";
            printHelp(options);
            return EXIT_FAILURE;
        }

        // Проверяем, что указана хотя бы одна команда
        if (!variables.count("current")
            && !variables.count("list")
            && !variables.count("upgrade-all")
            && !variables.count("upgrade")
            && !variables.count("downgrade"))
        {
            LOG_ERROR << "Ошибка: команда не указана!";
            printHelp(options);
            return EXIT_FAILURE;
        }

        // Инициализация базы данных
        db::DatabaseConfig config;
        config["database"] = variables["database"].as<std::string>();

        auto database = db::DatabaseFactory::create(db::DatabaseType::Sqlite);
        database->initialize(config);

        auto connection = database->connection();

        // Создание менеджера миграций
        db::MigrationManager manager(connection);
        manager.initializeSchemaTable();
        registerMigrations(manager);

        // Выполнение команд
        if (variables.count("current"))
        {
            showCurrentVersion(manager);
        }
        else if (variables.count("list"))
        {
            listMigrations(manager);
        }
        else if (variables.count("upgrade-all"))
        {
            LOG_INFO << "Обновление базы данных до последней версии...";
            manager.upgradeAll();
            LOG_INFO
                << "База данных успешно обновлена до версии "
                << manager.currentVersion();
        }
        else if (variables.count("upgrade"))
        {
            const unsigned int oldVersion = manager.currentVersion();
            const unsigned int maxVersion = manager.maxVersion();

            if (oldVersion >= maxVersion)
            {
                LOG_INFO << "Уже в последней версии (" << oldVersion << ")";
                return EXIT_SUCCESS;
            }

            LOG_INFO
                << "Обновление базы данных с версии " << oldVersion
                << " до " << (oldVersion + 1) << "...";
            manager.upgradeOne();
            LOG_INFO
                << "База данных успешно обновлена до версии "
                << manager.currentVersion();
        }
        else if (variables.count("downgrade"))
        {
            const unsigned int oldVersion = manager.currentVersion();

            if (oldVersion == 0)
            {
                LOG_INFO << "Уже в версии 0, дальнейшее понижение невозможно";
                return EXIT_SUCCESS;
            }

            LOG_INFO
                << "Понижение версии базы данных с " << oldVersion
                << " до " << (oldVersion - 1) << "...";
            manager.downgradeOne();
            LOG_INFO
                << "Версия базы данных успешно понижена до "
                << manager.currentVersion();
        }

        database->shutdown();
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка: " << e.what();
        return EXIT_FAILURE;
    }
}
