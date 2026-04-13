#define BOOST_TEST_MODULE "Vault Server Database Tests"

#include <filesystem>
#include <iostream>

#include <boost/test/unit_test.hpp>

/**
 * @brief Глобальная фикстура для настройки и очистки тестового окружения
 *
 * Выполняется один раз до начала всех тестов и после их завершения.
 * Создаёт директорию для тестовых БД и удаляет все временные файлы.
 *
 * Эта фикстура автоматически регистрируется в Boost.Test и не требует
 * явного вызова в тестах.
 */
struct GlobalTestFixture
{
    /**
     * @brief Конструктор: настройка окружения перед запуском тестов
     *
     * Создаёт директорию ./test_data (если её нет) и выводит информацию
     * о расположении тестовых файлов для отладки.
     */
    GlobalTestFixture()
    {
        std::cout << "=== Starting Database Tests ===" << std::endl;

        // Создаём директорию test_data, если она не существует
        std::error_code ec;
        std::filesystem::create_directories("./test_data", ec);
        if (ec)
        {
            std::cerr
                << "Warning: Could not create test_data directory: "
                << ec.message() << std::endl;
        }
        else
        {
            std::cout
                << "Test directory created/verified: "
                << std::filesystem::absolute("./test_data") << std::endl;
        }

        // Дополнительная проверка с абсолютным путём для надёжности
        auto abs_path = std::filesystem::absolute("./test_data");
        std::filesystem::create_directories(abs_path, ec);
    }

    /**
     * @brief Деструктор: очистка тестовых файлов после завершения всех тестов
     *
     * Удаляет все созданные в процессе тестирования файлы БД
     * и, если возможно, саму директорию test_data.
     * Ошибки игнорируются, так как это тестовая среда.
     */
    ~GlobalTestFixture()
    {
        std::cout << "=== Cleaning up test files ===" << std::endl;
        try
        {
            std::error_code ec;
            // Список всех тестовых файлов БД для удаления
            // (поддерживается вручную при добавлении новых тестов)
            std::filesystem::remove("./test_data/test.db", ec);
            std::filesystem::remove("./test_data/test2.db", ec);
            std::filesystem::remove("./test_data/transaction_test.db", ec);
            std::filesystem::remove("./test_data/connection_test.db", ec);
            std::filesystem::remove("./test_data/statement_test.db", ec);
            std::filesystem::remove("./test_data/resultset_test.db", ec);
            std::filesystem::remove("./test_data/integration_test.db", ec);
            std::filesystem::remove("./test_data/concurrent_test.db", ec);
            std::filesystem::remove("./test_data/error_test.db", ec);
            std::filesystem::remove("./test_data/test_init.db", ec);
            std::filesystem::remove("./test_data/test_init_twice.db", ec);
            std::filesystem::remove("./test_data/test_execute.db", ec);
            std::filesystem::remove("./test_data/test_invalid.db", ec);
            std::filesystem::remove("./test_data/test_query.db", ec);
            std::filesystem::remove("./test_data/test_empty_query.db", ec);
            std::filesystem::remove("./test_data/test_transaction_success.db", ec);
            std::filesystem::remove("./test_data/test_transaction_rollback.db", ec);
            std::filesystem::remove("./test_data/test_get_connection.db", ec);
            std::filesystem::remove("./test_data/test_fk.db", ec);
            std::filesystem::remove("./test_data/test_factory.db", ec);
            std::filesystem::remove("./test_data/test_conn_string.db", ec);

            // Удаляем директорию, если она пуста (игнорируем ошибку, если не пуста)
            std::filesystem::remove_all("./test_data", ec);
        }
        catch (...)
        {
            // Игнорируем ошибки при очистке
        }
    }
};

// Регистрируем глобальную фикстуру в Boost.Test
BOOST_GLOBAL_FIXTURE(GlobalTestFixture);
