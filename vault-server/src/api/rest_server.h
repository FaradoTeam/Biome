#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include "middleware/auth_middleware.h"

namespace farado
{
namespace server
{

/**
 * @brief Основной класс REST-сервера.
 *
 * Управляет HTTP-сервером, маршрутизацией запросов, аутентификацией
 * и обработкой запросов к API.
 */
class RestServer final
{
public:
    /**
     * @brief Конструктор REST-сервера.
     *
     * @param host Адрес для привязки ("0.0.0.0" - все интерфейсы)
     * @param port Порт для прослушивания
     */
    explicit RestServer(
        const std::string& host = "0.0.0.0",
        uint16_t port = 8080
    );
    ~RestServer();

    RestServer(const RestServer&) = delete;
    RestServer& operator=(const RestServer&) = delete;
    RestServer(RestServer&&) = delete;
    RestServer& operator=(RestServer&&) = delete;

    /**
     * @brief Инициализирует сервер: регистрирует маршруты и настраивает
     *        слушатель.
     * @return true при успешной инициализации
     */
    bool initialize();

    /**
     * @brief Запускает сервер и начинает прослушивание входящих соединений.
     * @return true при успешном запуске
     */
    bool start();

    /**
     * @brief Останавливает сервер и закрывает все соединения.
     */
    void stop();

    /**
     * @brief Проверяет, запущен ли сервер.
     * @return true если сервер запущен
     */
    bool isRunning() const { return m_isRunning; }

    /**
     * @brief Устанавливает middleware для аутентификации.
     *
     * @param middleware Указатель на объект AuthMiddleware
     */
    void setAuthMiddleware(std::shared_ptr<AuthMiddleware> middleware);

private:
    /**
     * @brief Тип функции-обработчика маршрута.
     *
     * @param request HTTP-запрос
     * @param userId Идентификатор аутентифицированного пользователя
     *               (может быть пустым для публичных маршрутов)
     */
    using RouteHandler = std::function<void(
        const web::http::http_request& request,
        const std::string& userId
    )>;

    /**
     * @brief Структура, описывающая маршрут.
     */
    struct RouteInfo
    {
        web::http::method method; ///< HTTP-метод
        std::string pathPattern; ///< Шаблон пути (регулярное выражение)
        RouteHandler handler; ///< Функция-обработчик
        bool isPublic { false }; ///< Публичный маршрут (без аутентификации)
    };

private:
    /**
     * @brief Регистрирует все маршруты API.
     */
    void registerRoutes();

    /**
     * @brief Настраивает HTTP-слушатель.
     */
    void setupListener();

    /**
     * @brief Основной обработчик входящих запросов.
     *
     * @param request HTTP-запрос
     */
    void handleRequest(web::http::http_request request);

    /**
     * @brief Применяет middleware аутентификации к запросу.
     *
     * @param request HTTP-запрос
     * @param userId Сюда будет записан ID пользователя при успехе
     * @return true если аутентификация пройдена
     */
    bool applyAuthMiddleware(
        const web::http::http_request& request,
        std::string& userId
    );

    /**
     * @brief Добавляет новый маршрут.
     *
     * @param method HTTP-метод (GET, POST, PUT, DELETE)
     * @param path Шаблон пути (может содержать регулярное выражение)
     * @param handler Функция-обработчик
     * @param isPublic Флаг: true - публичный маршрут (без аутентификации)
     */
    void addRoute(
        const web::http::method& method,
        const std::string& path,
        RouteHandler handler,
        bool isPublic = false
    );

    /**
     * @brief Находит маршрут, соответствующий запросу.
     *
     * @param method HTTP-метод
     * @param uriPath Путь из запроса
     * @param handler Сюда будет записан найденный обработчик
     * @param isPublic Сюда будет записан флаг публичности
     * @param params Сюда будут записаны параметры из пути
     * @return true если маршрут найден
     */
    bool matchRoute(
        const web::http::method& method,
        const std::string& uriPath,
        RouteHandler& handler,
        bool& isPublic,
        std::map<std::string, std::string>& params
    );

private:
    const std::string m_host; ///< Адрес сервера
    const uint16_t m_port; ///< Порт сервера

private:
    /// HTTP-слушатель
    std::unique_ptr<web::http::experimental::listener::http_listener> m_listener;

    /// Middleware для аутентификации
    std::shared_ptr<AuthMiddleware> m_authMiddleware;

    /// Флаг состояния сервера
    std::atomic<bool> m_isRunning { false };

    /// Список зарегистрированных маршрутов
    std::vector<RouteInfo> m_routes;
};

} // namespace server
} // namespace farado
