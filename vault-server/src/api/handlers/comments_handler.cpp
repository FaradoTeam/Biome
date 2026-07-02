#include <regex>

#include <cpprest/uri.h>

#include "common/dto/comment.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "comments_handler.h"

namespace server
{
namespace handlers
{

CommentsHandler::CommentsHandler(
    std::shared_ptr<services::ICommentService> commentService
)
    : m_commentService(std::move(commentService))
{
    if (!m_commentService)
    {
        LOG_WARN << "CommentsHandler инициализирован без CommentService";
    }
}

// ============================================================
// GET /comments
// ============================================================

void CommentsHandler::handleGetComments(
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
            LOG_WARN << "handleGetComments: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetComments: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int64_t> itemId = std::nullopt;
    if (params.count("itemId"))
    {
        try
        {
            itemId = std::stoll(params["itemId"]);
            if (itemId <= 0)
                itemId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetComments: неверный параметр itemId: " << params["itemId"];
        }
    }

    std::optional<int64_t> filterUserId = std::nullopt;
    if (params.count("userId"))
    {
        try
        {
            filterUserId = std::stoll(params["userId"]);
            if (filterUserId <= 0)
                filterUserId = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetComments: неверный параметр userId: " << params["userId"];
        }
    }

    // Фильтры по дате
    std::optional<common::DateTime> dateFrom = std::nullopt;
    if (params.count("dateFrom"))
    {
        try
        {
            int64_t timestamp = std::stoll(params["dateFrom"]);
            dateFrom = common::secondsToTimePoint(timestamp);
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetComments: неверный параметр dateFrom: " << params["dateFrom"];
        }
    }

    std::optional<common::DateTime> dateTo = std::nullopt;
    if (params.count("dateTo"))
    {
        try
        {
            int64_t timestamp = std::stoll(params["dateTo"]);
            dateTo = common::secondsToTimePoint(timestamp);
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetComments: неверный параметр dateTo: " << params["dateTo"];
        }
    }

    LOG_DEBUG
        << "GET /comments: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", itemId=" << (itemId.has_value() ? std::to_string(*itemId) : "none")
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none");

    try
    {
        auto commentsPage = m_commentService->getComments(
            page,
            pageSize,
            userId,
            itemId,
            filterUserId,
            dateFrom,
            dateTo
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < commentsPage.comments.size(); ++i)
        {
            items[i] = dto::toWebJson(commentsPage.comments[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(commentsPage.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка комментариев: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /comments/{id}
// ============================================================

void CommentsHandler::handleGetComment(
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

    const int64_t commentId = extractIdFromPath(request);
    if (commentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid comment ID");
        return;
    }

    LOG_DEBUG << "GET /comments/" << commentId << " from user " << userId;

    try
    {
        auto comment = m_commentService->getComment(commentId, userId);
        if (!comment)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Comment not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(comment->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении комментария " << commentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /items/{itemId}/comments
// ============================================================

void CommentsHandler::handleGetCommentsByItem(
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

    // Извлекаем itemId из пути: /items/{itemId}/comments
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/items/(\d+)/comments)");
    std::smatch matches;

    int64_t itemId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            itemId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
            return;
        }
    }

    if (itemId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid item ID");
        return;
    }

    LOG_DEBUG << "GET /items/" << itemId << "/comments from user " << userId;

    try
    {
        auto comments = m_commentService->getCommentsByItem(itemId, userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < comments.size(); ++i)
        {
            response[i] = dto::toWebJson(comments[i].toJson());
        }

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении комментариев для элемента " << itemId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /comments
// ============================================================

void CommentsHandler::handleCreateComment(
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

    LOG_DEBUG << "POST /comments from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::Comment comment(nlohmannJson);

                    // Устанавливаем автора
                    comment.userId = userId;

                    // Валидация обязательных полей
                    if (!comment.content.has_value() || comment.content->empty())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "Comment content is required");
                        return;
                    }

                    if (!comment.itemId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "itemId is required");
                        return;
                    }

                    auto created = m_commentService->createComment(comment, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create comment: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал комментарий id=" << *created->id
                        << " для элемента " << *created->itemId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании комментария: " << e.what();
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
// PUT /comments/{id}
// ============================================================

void CommentsHandler::handleUpdateComment(
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

    const int64_t commentId = extractIdFromPath(request);
    if (commentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid comment ID");
        return;
    }

    LOG_DEBUG << "PUT /comments/" << commentId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, commentId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = commentId;
                    dto::Comment comment(nlohmannJson);

                    auto updated = m_commentService->updateComment(comment, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или комментарий не найден
                        auto existing = m_commentService->getComment(commentId, userId);
                        if (!existing)
                        {
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Comment not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this comment"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил комментарий " << commentId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении комментария " << commentId << ": " << e.what();
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
// DELETE /comments/{id}
// ============================================================

void CommentsHandler::handleDeleteComment(
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

    const int64_t commentId = extractIdFromPath(request);
    if (commentId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid comment ID");
        return;
    }

    LOG_DEBUG << "DELETE /comments/" << commentId << " from user " << userId;

    try
    {
        auto result = m_commentService->deleteComment(commentId, userId);
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
            << " удалил комментарий " << commentId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении комментария " << commentId << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
