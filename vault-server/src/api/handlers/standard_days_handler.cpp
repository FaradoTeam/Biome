#include <regex>

#include <cpprest/uri.h>

#include "common/dto/standard_day.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "standard_days_handler.h"

namespace server
{
namespace handlers
{

StandardDaysHandler::StandardDaysHandler(
    std::shared_ptr<services::IStandardDayService> standardDayService
)
    : m_standardDayService(std::move(standardDayService))
{
    if (!m_standardDayService)
    {
        LOG_WARN << "StandardDaysHandler инициализирован без StandardDayService";
    }
}

// ============================================================
// GET /standard-days
// ============================================================

void StandardDaysHandler::handleGetStandardDays(
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

    LOG_DEBUG << "GET /standard-days from user " << userId;

    try
    {
        auto days = m_standardDayService->getAllStandardDays(userId);

        web::json::value response = web::json::value::array();
        for (size_t i = 0; i < days.size(); ++i)
        {
            response[i] = dto::toWebJson(days[i].toJson());
        }

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка стандартных дней: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// GET /standard-days/{weekDayNumber}
// ============================================================

void StandardDaysHandler::handleGetStandardDay(
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

    // Извлекаем weekDayNumber из пути: /standard-days/{weekDayNumber}
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/standard-days/(\d+))");
    std::smatch matches;

    int weekDayNumber = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            weekDayNumber = std::stoi(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid week day number");
            request.reply(resp);
            return;
        }
    }

    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Week day number must be between 0 and 6");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /standard-days/" << weekDayNumber << " from user " << userId;

    try
    {
        auto day = m_standardDayService->getStandardDayByWeekDay(weekDayNumber, userId);
        if (!day)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Standard day not found");
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
        LOG_ERROR << "Ошибка при получении стандартного дня " << weekDayNumber << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// PUT /standard-days/{weekDayNumber}
// ============================================================

void StandardDaysHandler::handleUpdateStandardDay(
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

    // Извлекаем weekDayNumber из пути
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/standard-days/(\d+))");
    std::smatch matches;

    int weekDayNumber = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            weekDayNumber = std::stoi(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid week day number");
            request.reply(resp);
            return;
        }
    }

    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Week day number must be between 0 and 6");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /standard-days/" << weekDayNumber << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, weekDayNumber](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    // Убеждаемся, что weekDayNumber в пути и в теле совпадают
                    nlohmannJson["weekDayNumber"] = weekDayNumber;
                    dto::StandardDay standardDay(nlohmannJson);

                    auto result = m_standardDayService->updateStandardDay(standardDay, userId);
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
                        << " обновил стандартный день " << weekDayNumber;

                    request.reply(web::http::status_codes::NoContent);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении стандартного дня " << weekDayNumber << ": " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

} // namespace handlers
} // namespace server
