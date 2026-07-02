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

void BoardsHandler::handleGetBoards(
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

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(boardsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка досок: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void BoardsHandler::handleGetBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
        return;
    }

    LOG_DEBUG << "GET /boards/" << boardId << " from user " << userId;

    try
    {
        auto board = m_boardService->getBoard(boardId, userId);
        if (!board)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Board not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(board->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении доски " << boardId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void BoardsHandler::handleCreateBoard(
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

                    if (!board.caption.has_value() || board.caption->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Board caption is required");
                        return;
                    }

                    if (!board.workflowId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "workflowId is required");
                        return;
                    }

                    if (!board.projectId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "projectId is required");
                        return;
                    }

                    auto created = m_boardService->createBoard(board, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create board: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал доску id=" << *created->id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании доски: " << e.what();
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

void BoardsHandler::handleUpdateBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
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
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Board not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this board"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил доску " << boardId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении доски " << boardId << ": " << e.what();
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

void BoardsHandler::handleDeleteBoard(
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

    const int64_t boardId = extractIdFromPath(request);
    if (boardId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid board ID");
        return;
    }

    LOG_DEBUG << "DELETE /boards/" << boardId << " from user " << userId;

    try
    {
        auto result = m_boardService->deleteBoard(boardId, userId);
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
            << " удалил доску " << boardId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении доски " << boardId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void BoardsHandler::handleGetBoardsByProject(
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid project ID");
            return;
        }
    }

    if (projectId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid project ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении досок проекта " << projectId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

void BoardsHandler::handleGetBoardsByPhase(
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid phase ID");
            return;
        }
    }

    if (phaseId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid phase ID");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении досок фазы " << phaseId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
