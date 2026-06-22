#include <cpprest/uri.h>

#include "common/dto/board.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "boards_handler.h"

namespace server
{
namespace handlers
{

BoardsHandler::BoardsHandler(
    std::shared_ptr<services::IBoardService> boardService
)
    : m_boardService(std::move(boardService))
{
    if (!m_boardService)
    {
        LOG_WARN << "BoardsHandler инициализирован без BoardService";
    }
}

// ============================================================
// GET /boards
// ============================================================

void BoardsHandler::handleGetBoards(
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
            LOG_WARN << "handleGetBoards: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetBoards: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> projectId = std::nullopt;
    if (params.count("projectId"))
    {
        try
        {
            projectId = std::stoll(params["projectId"]);
            if (projectId <= 0)
                projectId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoards: неверный параметр projectId: " << params["projectId"];
        }
    }

    std::optional<int64_t> phaseId = std::nullopt;
    if (params.count("phaseId"))
    {
        try
        {
            phaseId = std::stoll(params["phaseId"]);
            if (phaseId <= 0)
                phaseId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoards: неверный параметр phaseId: " << params["phaseId"];
        }
    }

    std::optional<int64_t> workflowId = std::nullopt;
    if (params.count("workflowId"))
    {
        try
        {
            workflowId = std::stoll(params["workflowId"]);
            if (workflowId <= 0)
                workflowId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetBoards: неверный параметр workflowId: " << params["workflowId"];
        }
    }

    LOG_DEBUG
        << "GET /boards: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", projectId=" << (projectId.has_value() ? std::to_string(*projectId) : "none")
        << ", phaseId=" << (phaseId.has_value() ? std::to_string(*phaseId) : "none")
        << ", workflowId=" << (workflowId.has_value() ? std::to_string(*workflowId) : "none");

    try
    {
        auto boardsPage = m_boardService->getBoards(
            page, pageSize, userId, projectId, phaseId, workflowId
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < boardsPage.boards.size(); ++i)
        {
            items[i] = dto::toWebJson(boardsPage.boards[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(boardsPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка досок: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /boards/{id}
// ============================================================

void BoardsHandler::handleGetBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid board ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /boards/" << boardId << " from user " << userId;

    try
    {
        auto board = m_boardService->getBoard(boardId, userId);
        if (!board)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Board not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(board->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении доски " << boardId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /boards
// ============================================================

void BoardsHandler::handleCreateBoard(
    const web::http::http_request& request,
    const std::string& userIdStr
)
{
    web::http::http_response errorResponse(web::http::status_codes::OK);

    LOG_DEBUG << "POST /boards";
    
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    LOG_DEBUG << "userIdOpt.has_value() " << userIdOpt.has_value();
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    LOG_DEBUG << "POST /boards from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Board board(nlohmannJson);

                    // Валидация обязательных полей
                    if (!board.caption.has_value() || board.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Board caption is required");
                        request.reply(resp);
                        return;
                    }

                    if (!board.workflowId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "workflowId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!board.projectId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "projectId is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_boardService->createBoard(board, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create board: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал доску id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании доски: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// PUT /boards/{id}
// ============================================================

void BoardsHandler::handleUpdateBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid board ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /boards/" << boardId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, boardId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = boardId;
                    dto::Board board(nlohmannJson);

                    auto updated = m_boardService->updateBoard(board, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или доска не найдена
                        auto existing = m_boardService->getBoard(boardId, userId);
                        if (!existing)
                        {
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Board not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this board");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил доску " << boardId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении доски " << boardId << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /boards/{id}
// ============================================================

void BoardsHandler::handleDeleteBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid board ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /boards/" << boardId << " from user " << userId;

    try
    {
        auto result = m_boardService->deleteBoard(boardId, userId);
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
            << " удалил доску " << boardId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении доски " << boardId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /projects/{projectId}/boards
// ============================================================

void BoardsHandler::handleGetBoardsByProject(
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

    // Извлекаем projectId из пути: /projects/{projectId}/boards
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/projects/(\d+)/boards)");
    std::smatch matches;

    int64_t projectId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            projectId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid project ID");
            request.reply(resp);
            return;
        }
    }

    if (projectId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid project ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /projects/" << projectId << "/boards from user " << userId;

    try
    {
        auto boards = m_boardService->getBoardsByProject(projectId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < boards.size(); ++i)
        {
            response[i] = dto::toWebJson(boards[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении досок проекта " << projectId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /phases/{phaseId}/boards
// ============================================================

void BoardsHandler::handleGetBoardsByPhase(
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

    // Извлекаем phaseId из пути: /phases/{phaseId}/boards
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/phases/(\d+)/boards)");
    std::smatch matches;

    int64_t phaseId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            phaseId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid phase ID");
            request.reply(resp);
            return;
        }
    }

    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /phases/" << phaseId << "/boards from user " << userId;

    try
    {
        auto boards = m_boardService->getBoardsByPhase(phaseId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < boards.size(); ++i)
        {
            response[i] = dto::toWebJson(boards[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении досок фазы " << phaseId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
