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

        response["items"] = items;
        response["totalCount"] = web::json::value::number(pageData.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка особых дней: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
        sendErrorResponse(resp, 400, "Invalid special day ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /special-days/" << id << " from user " << userId;

    try
    {
        auto day = m_specialDayService->getSpecialDay(id, userId);
        if (!day)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Special day not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(day->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении особого дня " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
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
    web::http::http_response errorResponse(web::http::status_codes::OK);
    auto userIdOpt = parseUserId(userIdStr, errorResponse);
    if (!userIdOpt.has_value())
    {
        request.reply(errorResponse);
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
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "date is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_specialDayService->createSpecialDay(specialDay, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create special day: insufficient permissions or invalid data"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал особый день id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании особого дня: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
        sendErrorResponse(resp, 400, "Invalid special day ID");
        request.reply(resp);
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
                            web::http::http_response resp(web::http::status_codes::NotFound);
                            sendErrorResponse(resp, 404, "Special day not found");
                            request.reply(resp);
                            return;
                        }

                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "Insufficient permissions to update this special day");
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил особый день " << id;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении особого дня " << id << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
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
        sendErrorResponse(resp, 400, "Invalid special day ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /special-days/" << id << " from user " << userId;

    try
    {
        auto result = m_specialDayService->deleteSpecialDay(id, userId);
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
            << " удалил особый день " << id;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении особого дня " << id << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
