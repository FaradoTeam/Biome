#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "base_handler.h"
#include "logic/iprivate_message_service.h"

namespace server::handlers
{

/**
 * @brief Обработчик запросов для работы с личными сообщениями.
 */
class PrivateMessagesHandler final : public BaseHandler
{
public:
    explicit PrivateMessagesHandler(
        std::shared_ptr<services::IPrivateMessageService> service
    );

    /**
     * @brief Получает список личных сообщений с пагинацией и фильтрацией.
     * GET /private-messages
     */
    void handleGetMessages(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает личное сообщение по ID.
     * GET /private-messages/{id}
     */
    void handleGetMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает переписку с пользователем.
     * GET /private-messages/conversation/{userId}
     */
    void handleGetConversation(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Отправляет личное сообщение.
     * POST /private-messages
     */
    void handleSendMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Отмечает сообщение как прочитанное.
     * PUT /private-messages/{id}/view
     */
    void handleMarkAsViewed(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет личное сообщение.
     * DELETE /private-messages/{id}
     */
    void handleDeleteMessage(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает количество непрочитанных сообщений.
     * GET /private-messages/unviewed/count
     */
    void handleCountUnviewed(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IPrivateMessageService> m_service;
};

} // namespace server::handlers
