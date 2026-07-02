#include <regex>

#include <cpprest/uri.h>

#include "common/dto/special_day.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "special_days_handler.h"

namespace server
{
namespace handlers
{

SpecialDaysHandler::SpecialDaysHandler(
    std::shared_ptr<services::ISpecialDayService> specialDayService
)
    : m_specialDayService(std::move(specialDayService))
{
    if (!m_specialDayService)
    {
        LOG_WARN << "SpecialDaysHandler инициализирован без SpecialDayService";
    }
}

// ============================================================
// GET /special-days
// ============================================================

void SpecialDaysHandler::handleGetSpecialDays(
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
            LOG_WARN << "handleGetSpecialDays: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetSpecialDays: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    // Фильтры
    std::optional<int> year = std::nullopt;
    if (params.count("year"))
    {
        try
        {
            year = std::stoi(params["year"]);
            if (*year < 1970 || *year > 2100)
                year = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetSpecialDays: неверный параметр year: " << params["year"];
        }
    }

    std::optional<int> month = std::nullopt;
    if (params.count("month"))
    {
        try
        {
            month = std::stoi(params["month"]);
            if (*month < 1 || *month > 12)
                month = std::nullopt;
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetSpecialDays: неверный параметр month: " << params["month"];
        }
    }

    LOG_DEBUG
        << "GET /special-days: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", year=" << (year.has_value() ? std::to_string(*year) : "none")
        << ", month=" << (month.has_value() ? std::to_string(*month) : "none");

    try
    {
        auto pageData = m_specialDayService->getSpecialDays(
            page, pageSize, userId, year, month
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
        LOG_ERROR << "Ошибка при получении списка особых дней: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// GET /special-days/{id}
// ============================================================

void SpecialDaysHandler::handleGetSpecialDay(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid special day ID");
        return;
    }

    LOG_DEBUG << "GET /special-days/" << id << " from user " << userId;

    try
    {
        auto day = m_specialDayService->getSpecialDay(id, userId);
        if (!day)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Special day not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(day->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении особого дня " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

// ============================================================
// POST /special-days
// ============================================================

void SpecialDaysHandler::handleCreateSpecialDay(
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

    LOG_DEBUG << "POST /special-days from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::SpecialDay specialDay(nlohmannJson);

                    // Валидация обязательных полей
                    if (!specialDay.date.has_value())
                    {
                        sendErrorResponse(request, web::http::status_codes::BadRequest, "date is required");
                        return;
                    }

                    auto created = m_specialDayService->createSpecialDay(specialDay, userId);
                    if (!created)
                    {
                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Cannot create special day: insufficient permissions or invalid data"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал особый день id=" << *created->id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании особого дня: " << e.what();
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
// PUT /special-days/{id}
// ============================================================

void SpecialDaysHandler::handleUpdateSpecialDay(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid special day ID");
        return;
    }

    LOG_DEBUG << "PUT /special-days/" << id << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, id](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что ID в пути и в теле совпадают
                    nlohmannJson["id"] = id;
                    dto::SpecialDay specialDay(nlohmannJson);

                    auto updated = m_specialDayService->updateSpecialDay(specialDay, userId);
                    if (!updated)
                    {
                        // Пытаемся определить причину: нет прав или день не найден
                        auto existing = m_specialDayService->getSpecialDay(id, userId);
                        if (!existing)
                        {
                            sendErrorResponse(request, web::http::status_codes::NotFound, "Special day not found");
                            return;
                        }

                        sendErrorResponse(
                            request,
                            web::http::status_codes::Forbidden,
                            "Insufficient permissions to update this special day"
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил особый день " << id;

                    sendJsonResponse(
                        request,
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении особого дня " << id << ": " << e.what();
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
// DELETE /special-days/{id}
// ============================================================

void SpecialDaysHandler::handleDeleteSpecialDay(
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
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid special day ID");
        return;
    }

    LOG_DEBUG << "DELETE /special-days/" << id << " from user " << userId;

    try
    {
        auto result = m_specialDayService->deleteSpecialDay(id, userId);
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
            << " удалил особый день " << id;

        web::http::http_response response(web::http::status_codes::NoContent);
        sendResponse(request, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении особого дня " << id << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
    }
}

} // namespace handlers
} // namespace server
