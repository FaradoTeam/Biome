#pragma once

#include <map>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с элементами (CRUD операции).
 *
 * Предоставляет методы для создания, чтения, обновления и удаления элементов.
 * Все методы, кроме специально отмеченных, требуют аутентификации пользователя.
 */
class ItemsHandler final : public BaseHandler
{
public:
    ItemsHandler();

    /**
     * @brief Получает список элементов с пагинацией.
     *
     * Поддерживает query-параметры:
     * - page: номер страницы (по умолчанию 1)
     * - pageSize: количество элементов на странице (по умолчанию 20)
     *
     * @param request HTTP-запрос от клиента
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleGetItems(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает конкретный элемент по его ID.
     *
     * @param request HTTP-запрос от клиента (должен содержать ID в пути)
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleGetItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создает новый элемент.
     *
     * Ожидает JSON-тело запроса с полем "caption" (обязательно).
     *
     * @param request HTTP-запрос от клиента
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleCreateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующий элемент.
     *
     * @param request HTTP-запрос от клиента
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleUpdateItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет элемент.
     *
     * @param request HTTP-запрос от клиента
     * @param userId Идентификатор аутентифицированного пользователя
     */
    void handleDeleteItem(
        const web::http::http_request& request,
        const std::string& userId
    );

};

} // namespace handlers
} // namespace server
