#include <regex>

#include <cpprest/uri.h>

#include "common/dto/board_column.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "board_columns_handler.h"

namespace server
{
namespace handlers
{

BoardColumnsHandler::BoardColumnsHandler(
    std::shared_ptr<services::IBoardColumnService> boardColumnService
)
    : m_boardColumnService(std::move(boardColumnService))
{
    if (!m_boardColumnService)
    {
        LOG_WARN << "BoardColumnsHandler инициализирован без BoardColumnService";
    }
}

// ============================================================
// GET /board-columns
// ============================================================

void BoardColumnsHandler::handleGetBoardColumns(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    auto params = extractQueryParams(request);

    // Параметры пагинации
    int page = 1;
    if (params.count("page"))
    {
        try
        {
            page = std::stoi(params["page"]);
            if (page < 1)
                page = 1;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoardColumns: неверный параметр page: " << params["page"];
        }
    }

    int pageSize = 20;
    if (params.count("pageSize"))
    {
        try
        {
            pageSize = std::stoi(params["pageSize"]);
            if (pageSize < 1)
                pageSize = 1;
            if (pageSize > 100)
                pageSize = 100;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoardColumns: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> boardId = std::nullopt;
    if (params.count("boardId"))
    {
        try
        {
            boardId = std::stoll(params["boardId"]);
            if (boardId <= 0)
                boardId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoardColumns: неверный параметр boardId: " << params["boardId"];
        }
    }

    std::optional<int64_t> stateId = std::nullopt;
    if (params.count("stateId"))
    {
        try
        {
            stateId = std::stoll(params["stateId"]);
            if (stateId <= 0)
                stateId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoardColumns: неверный параметр stateId: " << params["stateId"];
        }
    }

    LOG_DEBUG
        << "GET /board-columns: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", boardId=" << (boardId.has_value() ? std::to_string(*boardId) : "none")
        << ", stateId=" << (stateId.has_value() ? std::to_string(*stateId) : "none");

    try
    {
        auto columnsPage = m_boardColumnService->getBoardColumns(
            page, pageSize, userId, boardId, stateId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < columnsPage.columns.size(); ++i)
        {
            items[i] = dto::toWebJson(columnsPage.columns[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(columnsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка колонок досок: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /board-columns/{id}
// ============================================================

void BoardColumnsHandler::handleGetBoardColumn(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid column ID");
        return;
    }

    LOG_DEBUG << "GET /board-columns/" << columnId << " from user " << userId;

    try
    {
        auto column = m_boardColumnService->getBoardColumn(columnId, userId);
        if (!column)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Board column not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(column->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении колонки доски " << columnId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /boards/{boardId}/columns
// ============================================================

void BoardColumnsHandler::handleGetColumnsByBoard(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем boardId из пути: /boards/{boardId}/columns
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/boards/(\d+)/columns)");
    std::smatch matches;

    int64_t boardId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            boardId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
            return;
        }
    }

    if (boardId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
        return;
    }

    LOG_DEBUG << "GET /boards/" << boardId << "/columns from user " << userId;

    try
    {
        auto columns = m_boardColumnService->getColumnsByBoard(boardId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < columns.size(); ++i)
        {
            response[i] = dto::toWebJson(columns[i].toJson());
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении колонок доски " << boardId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /boards/{boardId}/columns
// ============================================================

void BoardColumnsHandler::handleCreateBoardColumn(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем boardId из пути: /boards/{boardId}/columns
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/boards/(\d+)/columns)");
    std::smatch matches;

    int64_t boardId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            boardId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
            return;
        }
    }

    if (boardId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
        return;
    }

    LOG_DEBUG << "POST /boards/" << boardId << "/columns from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, boardId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что boardId в пути и в теле совпадают
                    nlohmannJson["boardId"] = boardId;
                    dto::BoardColumn column(nlohmannJson);

                    // Валидация обязательных полей
                    if (!column.stateId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "stateId is required");
                        return;
                    }

                    if (!column.orderNumber.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "orderNumber is required");
                        return;
                    }

                    auto created = m_boardColumnService->createBoardColumn(column, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create board column: insufficient permissions, duplicate, or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал колонку доски id=" << *created->id
                        << " для доски " << boardId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании колонки доски: " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

// ============================================================
// PUT /board-columns/{id}
// ============================================================

void BoardColumnsHandler::handleUpdateBoardColumn(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid column ID");
        return;
    }

    LOG_DEBUG << "PUT /board-columns/" << columnId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, columnId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = columnId;
                    dto::BoardColumn column(nlohmannJson);

                    auto updated = m_boardColumnService->updateBoardColumn(column, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или колонка не найдена
                        auto existing = m_boardColumnService->getBoardColumn(columnId, userId);
                        if (!existing)
                        {
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Board column not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot update column: insufficient permissions, duplicate, or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил колонку доски " << columnId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении колонки доски " << columnId << ": " << e.what();
                    sendErrorResponse(
                        request,
                        web::http::status_codes::BadRequest,
                        std::string("Invalid request: ") + e.what()
                    );
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /board-columns/{id}
// ============================================================

void BoardColumnsHandler::handleDeleteBoardColumn(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid column ID");
        return;
    }

    LOG_DEBUG << "DELETE /board-columns/" << columnId << " from user " << userId;

    try
    {
        auto result = m_boardColumnService->deleteBoardColumn(columnId, userId);
        if (!result.success)
        {
            sendErrorResponse(
                request,
                static_cast<web::http::status_code>(result.errorCode),
                result.errorMessage
            );
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил колонку доски " << columnId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении колонки доски " << columnId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
