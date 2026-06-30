#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_action_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с действиями пользователя.
 */
class UserActionsHandler final : public BaseHandler
{
public:
    explicit UserActionsHandler(std::shared_ptr<services::IUserActionService> actionService);

    /**
     * @brief Получает список действий пользователя с пагинацией и фильтрацией.
     * GET /user-actions
     */
    void handleGetActions(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает действие по ID.
     * GET /user-actions/{id}
     */
    void handleGetAction(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новое действие.
     * POST /user-actions
     */
    void handleCreateAction(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет действие.
     * DELETE /user-actions/{id}
     */
    void handleDeleteAction(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IUserActionService> m_actionService;
};

} // namespace handlers
} // namespace server
