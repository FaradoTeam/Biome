#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "base_handler.h"
#include "logic/iteam_message_service.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы с сообщениями в командах.
 */
class TeamMessagesHandler final : public BaseHandler
{
public:
    explicit TeamMessagesHandler(
        std::shared_ptr<services::ITeamMessageService> service
    );

    /**
     * @brief Получает список сообщений с пагинацией и фильтрацией.
     * GET /team-messages
     */
    void handleGetMessages(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает сообщение по ID.
     * GET /team-messages/{id}
     */
    void handleGetMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все сообщения в команде.
     * GET /teams/{teamId}/messages
     */
    void handleGetTeamMessages(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Отправляет сообщение в команду.
     * POST /team-messages
     */
    void handleSendMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет сообщение из команды.
     * DELETE /team-messages/{id}
     */
    void handleDeleteMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::ITeamMessageService> m_service;
};

} // namespace server::handlers
