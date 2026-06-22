#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iboard_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с досками (CRUD операции).
 */
class BoardsHandler final : public BaseHandler
{
public:
    explicit BoardsHandler(std::shared_ptr<services::IBoardService> boardService);

    /**
     * @brief Получает список досок с пагинацией и фильтрацией.
     * GET /boards
     */
    void handleGetBoards(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает доску по ID.
     * GET /boards/{id}
     */
    void handleGetBoard(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую доску.
     * POST /boards
     */
    void handleCreateBoard(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующую доску.
     * PUT /boards/{id}
     */
    void handleUpdateBoard(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет доску.
     * DELETE /boards/{id}
     */
    void handleDeleteBoard(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает доски для проекта.
     * GET /projects/{projectId}/boards
     */
    void handleGetBoardsByProject(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает доски для фазы.
     * GET /phases/{phaseId}/boards
     */
    void handleGetBoardsByPhase(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IBoardService> m_boardService;
};

} // namespace handlers
} // namespace server
