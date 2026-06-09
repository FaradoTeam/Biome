#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iitem_user_state_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с историей состояний элементов.
 */
class ItemUserStatesHandler final : public BaseHandler
{
public:
    explicit ItemUserStatesHandler(
        std::shared_ptr<services::IItemUserStateService> service
    );

    /**
     * @brief Получает список записей истории состояний.
     * GET /api/items/{itemId}/user-states
     * GET /api/items/user-states (с фильтрацией)
     */
    void handleGetItemUserStates(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает запись истории по ID.
     * GET /api/items/user-states/{id}
     */
    void handleGetItemUserState(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает последнюю запись истории для элемента.
     * GET /api/items/{itemId}/user-states/last
     */
    void handleGetLastItemUserState(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую запись истории состояния.
     * POST /api/items/{itemId}/user-states
     */
    void handleCreateItemUserState(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет запись истории.
     * DELETE /api/items/user-states/{id}
     */
    void handleDeleteItemUserState(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IItemUserStateService> m_service;
};

} // namespace handlers
} // namespace server
