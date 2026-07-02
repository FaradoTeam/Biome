#include <regex>

#include <cpprest/uri.h>

#include "common/dto/user_day.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "user_days_handler.h"

namespace server
{
namespace handlers
{

UserDaysHandler::UserDaysHandler(
    std::shared_ptr<services::IUserDayService> userDayService
)
    : m_userDayService(std::move(userDayService))
{
    if (!m_userDayService)
    {
        LOG_WARN << "UserDaysHandler инициализирован без UserDayService";
    }
}

// ============================================================
// GET /user-days
// ============================================================

void UserDaysHandler::handleGetUserDays(
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
            LOG_WARN << "handleGetUserDays: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetUserDays: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
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
            LOG_WARN << "handleGetUserDays: неверный параметр userId: " << params["userId"];
        }
    }

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
            LOG_WARN << "handleGetUserDays: неверный параметр dateFrom: " << params["dateFrom"];
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
            LOG_WARN << "handleGetUserDays: неверный параметр dateTo: " << params["dateTo"];
        }
    }

    LOG_DEBUG
        << "GET /user-days: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", filterUserId=" << (filterUserId.has_value() ? std::to_string(*filterUserId) : "none");

    try
    {
        auto pageData = m_userDayService->getUserDays(
            page, pageSize, userId, filterUserId, dateFrom, dateTo
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < pageData.days.size(); ++i)
        {
            items[i] = dto::toWebJson(pageData.days[i].toJson());
        }

        response[U("items")] = items;
        response[U("totalCount")] = web::json::value::number(pageData.totalCount);
        response[U("page")] = web::json::value::number(page);
        response[U("pageSize")] = web::json::value::number(pageSize);

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка пользовательских дней: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /user-days/{id}
// ============================================================

void UserDaysHandler::handleGetUserDay(
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

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user day ID");
        return;
    }

    LOG_DEBUG << "GET /user-days/" << id << " from user " << userId;

    try
    {
        auto userDay = m_userDayService->getUserDay(id, userId);
        if (!userDay)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "User day not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(userDay->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении пользовательского дня " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /users/{userId}/days/{date}
// ============================================================

void UserDaysHandler::handleGetUserDayByUserAndDate(
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
    const int64_t currentUserId = *userIdOpt;

    // Извлекаем параметры из пути: /users/{userId}/days/{date}
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/users/(\d+)/days/(\d+))");
    std::smatch matches;

    int64_t targetUserId = -1;
    int64_t dateTimestamp = -1;

    if (std::regex_search(path, matches, pattern) && matches.size() >= 3)
    {
        try
        {
            targetUserId = std::stoll(matches[1].str());
            dateTimestamp = std::stoll(matches[2].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID or date");
            return;
        }
    }

    if (targetUserId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    auto date = common::secondsToTimePoint(dateTimestamp);

    LOG_DEBUG
        << "GET /users/" << targetUserId << "/days/" << dateTimestamp
        << " from user " << currentUserId;

    try
    {
        auto userDay = m_userDayService->getUserDayByUserAndDate(
            targetUserId, date, currentUserId
        );
        if (!userDay)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "User day not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(userDay->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка при получении пользовательского дня: "
            << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /user-days
// ============================================================

void UserDaysHandler::handleCreateUserDay(
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
    const int64_t currentUserId = *userIdOpt;

    LOG_DEBUG << "POST /user-days from user " << currentUserId;

    request
        .extract_json()
        .then(
            [this, request, currentUserId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::UserDay userDay(nlohmannJson);

                    // Валидация обязательных полей
                    if (!userDay.userId.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "userId is required");
                        return;
                    }

                    if (!userDay.date.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "date is required");
                        return;
                    }

                    auto created = m_userDayService->createUserDay(userDay, currentUserId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create user day: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << currentUserId
                        << " создал пользовательский день id=" << *created->id
                        << " для пользователя " << *created->userId;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании пользовательского дня: " << e.what();
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
// PUT /user-days/{id}
// ============================================================

void UserDaysHandler::handleUpdateUserDay(
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
    const int64_t currentUserId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user day ID");
        return;
    }

    LOG_DEBUG << "PUT /user-days/" << id << " from user " << currentUserId;

    request
        .extract_json()
        .then(
            [this, request, currentUserId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = id;
                    dto::UserDay userDay(nlohmannJson);

                    auto updated = m_userDayService->updateUserDay(userDay, currentUserId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или день не найден
                        auto existing = m_userDayService->getUserDay(id, currentUserId);
                        if (!existing)
                        {
                            sendErrorResponse(request, web::http::status_codes::NotFound, "User day not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this user day"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << currentUserId
                        << " обновил пользовательский день " << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR
                        << "Ошибка при обновлении пользовательского дня " << id
                        << ": " << e.what();
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
// DELETE /user-days/{id}
// ============================================================

void UserDaysHandler::handleDeleteUserDay(
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
    const int64_t currentUserId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user day ID");
        return;
    }

    LOG_DEBUG << "DELETE /user-days/" << id << " from user " << currentUserId;

    try
    {
        auto result = m_userDayService->deleteUserDay(id, currentUserId);
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
            << "Пользователь " << currentUserId
            << " удалил пользовательский день " << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении пользовательского дня " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// DELETE /users/{userId}/days
// ============================================================

void UserDaysHandler::handleDeleteUserDaysByUser(
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
    const int64_t currentUserId = *userIdOpt;

    // Извлекаем userId из пути: /users/{userId}/days
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/users/(\d+)/days)");
    std::smatch matches;

    int64_t targetUserId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            targetUserId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
            return;
        }
    }

    if (targetUserId <= 0)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid user ID");
        return;
    }

    LOG_DEBUG
        << "DELETE /users/" << targetUserId << "/days"
        << " from user " << currentUserId;

    try
    {
        int64_t deleted = m_userDayService->deleteUserDaysByUser(targetUserId, currentUserId);

        LOG_INFO
            << "Пользователь " << currentUserId
            << " удалил " << deleted << " дней для пользователя " << targetUserId;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка при удалении дней пользователя " << targetUserId
            << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
