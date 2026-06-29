#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/icomment_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с комментариями.
 */
class CommentsHandler final : public BaseHandler
{
public:
    explicit CommentsHandler(std::shared_ptr<services::ICommentService> commentService);

    /**
     * @brief Получает список комментариев с пагинацией.
     * GET /comments
     */
    void handleGetComments(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает комментарий по ID.
     * GET /comments/{id}
     */
    void handleGetComment(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новый комментарий.
     * POST /comments
     */
    void handleCreateComment(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующий комментарий.
     * PUT /comments/{id}
     */
    void handleUpdateComment(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет комментарий.
     * DELETE /comments/{id}
     */
    void handleDeleteComment(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает комментарии для элемента.
     * GET /items/{itemId}/comments
     */
    void handleGetCommentsByItem(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::ICommentService> m_commentService;
};

} // namespace handlers
} // namespace server
