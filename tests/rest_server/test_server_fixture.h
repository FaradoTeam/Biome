// ============================================================================
// Фикстура для тестирования REST сервера Vault
// Предоставляет утилиты для запуска/остановки сервера, создания клиентов,
// генерации тестовых токенов и работы с JSON
// ============================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include "vault-server/src/api/middleware/impl/auth_middleware.h"
#include "vault-server/src/api/rest_server.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::test
{

// ============================================================================
// КОНФИГУРАЦИЯ ТЕСТОВОГО СЕРВЕРА
// ============================================================================
// Используем localhost и свободный порт 18080, чтобы не конфликтовать
// с возможными другими серверами на стандартных портах
// ============================================================================

const std::string TEST_HOST = "127.0.0.1";
const uint16_t TEST_PORT = 18080;
const std::string TEST_BASE_URL = "http://" + TEST_HOST + ":" + std::to_string(TEST_PORT);
const std::string TEST_SECRET_KEY = "your-very-long-secret-key-that-is-at-least-32-bytes-long!";

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ РАБОТЫ С JSON
// ============================================================================

/**
 * @brief Извлекает целочисленное поле из JSON-объекта
 * @param obj JSON-объект
 * @param key Имя поля
 * @return Значение поля как int64_t
 * @throws std::exception Если поле отсутствует или имеет неверный тип
 */
inline int64_t getJsonInt(const json::value& obj, const std::string& key)
{
    return obj.at(key).as_integer();
}

/**
 * @brief Извлекает строковое поле из JSON-объекта
 * @param obj JSON-объект
 * @param key Имя поля
 * @return Значение поля как std::string
 */
inline std::string getJsonString(const json::value& obj, const std::string& key)
{
    return obj.at(key).as_string();
}

/**
 * @brief Проверяет наличие поля в JSON-объекте
 * @param obj JSON-объект
 * @param key Имя поля
 * @return true если поле существует, false иначе
 */
inline bool hasJsonField(const json::value& obj, const std::string& key)
{
    return obj.has_field(key);
}

// ============================================================================
// ТЕСТОВАЯ ФИКСТУРА
// ============================================================================
// Управляет жизненным циклом сервера: автоматически запускает его перед тестом
// и останавливает после завершения. Предоставляет удобные методы для
// создания клиентов и генерации тестовых токенов.
// ============================================================================

/**
 * @brief Фикстура для тестирования REST сервера
 *
 * Использование:
 * @code
 * BOOST_FIXTURE_TEST_CASE(my_test, TestServerFixture)
 * {
 *     auto client = createClient();
 *     std::string token = getTestToken();
 *     // ... тестирование
 * }
 * @endcode
 */
struct TestServerFixture
{
    /**
     * @brief Конструктор — запускает сервер перед тестом
     *
     * Создает экземпляр RestServer на тестовом хосте и порту,
     * настраивает middleware аутентификации и запускает сервер.
     * После запуска делает небольшую паузу для инициализации.
     */
    TestServerFixture()
        : m_server(std::make_shared<RestServer>(TEST_HOST, TEST_PORT))
    {
        auto authMiddleware = std::make_shared<AuthMiddleware>(TEST_SECRET_KEY);
        m_server->setAuthMiddleware(authMiddleware);
        m_server->initialize();
        m_server->start();

        // Даем серверу время на полный запуск (особенно важно для асинхронных операций)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /**
     * @brief Деструктор — останавливает сервер после теста
     *
     * Гарантирует корректное завершение работы сервера и освобождение ресурсов
     */
    ~TestServerFixture()
    {
        if (m_server)
        {
            m_server->stop();
        }
    }

    /**
     * @brief Возвращает указатель на экземпляр сервера
     * @return shared_ptr<RestServer>
     */
    std::shared_ptr<RestServer> getServer() const { return m_server; }

    /**
     * @brief Генерирует валидный JWT токен для тестов
     * @return Строка с JWT токеном, действительным в течение 1 часа
     */
    std::string getTestToken() const
    {
        AuthMiddleware auth(TEST_SECRET_KEY);
        return auth.generateToken("test_user_123", 3600);
    }

    /**
     * @brief Создает HTTP-клиент, настроенный на тестовый сервер
     * @return http_client объект для отправки запросов
     */
    http_client createClient() const
    {
        return http_client { uri(TEST_BASE_URL) };
    }

    std::shared_ptr<RestServer> m_server; ///< Экземпляр тестируемого сервера
};

} // namespace server::test
