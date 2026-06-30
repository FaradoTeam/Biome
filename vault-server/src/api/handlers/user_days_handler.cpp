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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка пользовательских дней: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t userId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user day ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /user-days/" << id << " from user " << userId;

    try
    {
        auto userDay = m_userDayService->getUserDay(id, userId);
        if (!userDay)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "User day not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(userDay->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении пользовательского дня " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid user ID or date");
            request.reply(resp);
            return;
        }
    }

    if (targetUserId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
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
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "User day not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(userDay->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка при получении пользовательского дня: "
            << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "userId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!userDay.date.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "date is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_userDayService->createUserDay(userDay, currentUserId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create user day: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << currentUserId
                        << " создал пользовательский день id=" << *created->id
                        << " для пользователя " << *created->userId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании пользовательского дня: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t currentUserId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user day ID");
        request.reply(resp);
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
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "User day not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Insufficient permissions to update this user day"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << currentUserId
                        << " обновил пользовательский день " << id;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR
                        << "Ошибка при обновлении пользовательского дня " << id
                        << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
        return;
    }
    const int64_t currentUserId = *userIdOpt;

    const int64_t id = extractIdFromPath(request);
    if (id <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user day ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /user-days/" << id << " from user " << currentUserId;

    try
    {
        auto result = m_userDayService->deleteUserDay(id, currentUserId);
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
            << "Пользователь " << currentUserId
            << " удалил пользовательский день " << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении пользовательского дня " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid user ID");
            request.reply(resp);
            return;
        }
    }

    if (targetUserId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid user ID");
        request.reply(resp);
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

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Ошибка при удалении дней пользователя " << targetUserId
            << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
