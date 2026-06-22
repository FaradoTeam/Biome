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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(columnsPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка колонок досок: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid column ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /board-columns/" << columnId << " from user " << userId;

    try
    {
        auto column = m_boardColumnService->getBoardColumn(columnId, userId);
        if (!column)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Board column not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(column->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении колонки доски " << columnId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем boardId из пути: /boards/{boardId}/columns
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/boards/(\d+)/columns)");
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid board ID");
            request.reply(resp);
            return;
        }
    }

    if (boardId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid board ID");
        request.reply(resp);
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

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении колонок доски " << boardId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    // Извлекаем boardId из пути: /boards/{boardId}/columns
    std::string path = web::uri::decode(request.relative_uri().path());
    std::regex pattern(R"(/boards/(\d+)/columns)");
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid board ID");
            request.reply(resp);
            return;
        }
    }

    if (boardId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid board ID");
        request.reply(resp);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "stateId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!column.orderNumber.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "orderNumber is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_boardColumnService->createBoardColumn(column, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create board column: insufficient permissions, duplicate, or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал колонку доски id=" << *created->id
                        << " для доски " << boardId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании колонки доски: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid column ID");
        request.reply(resp);
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
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Board column not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot update column: insufficient permissions, duplicate, or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил колонку доски " << columnId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении колонки доски " << columnId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t columnId = extractIdFromPath(request);
    if (columnId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid column ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /board-columns/" << columnId << " from user " << userId;

    try
    {
        auto result = m_boardColumnService->deleteBoardColumn(columnId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO
            << "Пользователь " << userId
            << " удалил колонку доски " << columnId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении колонки доски " << columnId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
