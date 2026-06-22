#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iboard_column_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с колонками досок (CRUD операции).
 */
class BoardColumnsHandler final : public BaseHandler
{
public:
    explicit BoardColumnsHandler(
        std::shared_ptr<services::IBoardColumnService> boardColumnService
    );

    /**
     * @brief Получает список колонок с пагинацией и фильтрацией.
     * GET /board-columns
     */
    void handleGetBoardColumns(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает колонку по ID.
     * GET /board-columns/{id}
     */
    void handleGetBoardColumn(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает все колонки для доски.
     * GET /boards/{boardId}/columns
     */
    void handleGetColumnsByBoard(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новую колонку доски.
     * POST /boards/{boardId}/columns
     */
    void handleCreateBoardColumn(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет существующую колонку.
     * PUT /board-columns/{id}
     */
    void handleUpdateBoardColumn(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет колонку доски.
     * DELETE /board-columns/{id}
     */
    void handleDeleteBoardColumn(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IBoardColumnService> m_boardColumnService;
};

} // namespace handlers
} // namespace server
