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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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

        sendJsonResponse(request, web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка стандартных дней: " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid week day number");
            return;
        }
    }

    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Week day number must be between 0 and 6");
        return;
    }

    LOG_DEBUG << "GET /standard-days/" << weekDayNumber << " from user " << userId;

    try
    {
        auto day = m_standardDayService->getStandardDayByWeekDay(weekDayNumber, userId);
        if (!day)
        {
            sendErrorResponse(request, web::http::status_codes::NotFound, "Standard day not found");
            return;
        }

        sendJsonResponse(request, web::http::status_codes::OK, dto::toWebJson(day->toJson()));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении стандартного дня " << weekDayNumber << ": " << e.what();
        sendErrorResponse(request, web::http::status_codes::InternalError, "Internal server error");
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
    auto userIdOpt = parseUserId(userIdStr);
    if (!userIdOpt.has_value())
    {
        sendErrorResponse(request, web::http::status_codes::Unauthorized, "User not authenticated");
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
            sendErrorResponse(request, web::http::status_codes::BadRequest, "Invalid week day number");
            return;
        }
    }

    if (weekDayNumber < 0 || weekDayNumber > 6)
    {
        sendErrorResponse(request, web::http::status_codes::BadRequest, "Week day number must be between 0 and 6");
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
                        sendErrorResponse(
                            request,
                            static_cast<web::http::status_code>(result.errorCode),
                            result.errorMessage
                        );
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил стандартный день " << weekDayNumber;

                    web::http::http_response response(web::http::status_codes::NoContent);
                    sendResponse(request, response);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении стандартного дня " << weekDayNumber << ": " << e.what();
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

} // namespace handlers
} // namespace server
