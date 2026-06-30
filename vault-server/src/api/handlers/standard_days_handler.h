#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/istandard_day_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы со стандартными днями.
 */
class StandardDaysHandler final : public BaseHandler
{
public:
    explicit StandardDaysHandler(
        std::shared_ptr<services::IStandardDayService> standardDayService
    );

    /**
     * @brief Получает список всех стандартных дней.
     * GET /standard-days
     */
    void handleGetStandardDays(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает стандартный день по номеру дня недели.
     * GET /standard-days/{weekDayNumber}
     */
    void handleGetStandardDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет стандартный день.
     * PUT /standard-days/{weekDayNumber}
     */
    void handleUpdateStandardDay(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IStandardDayService> m_standardDayService;
};

} // namespace handlers
} // namespace server
