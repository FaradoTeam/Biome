#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/ispecial_day_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с особыми днями.
 */
class SpecialDaysHandler final : public BaseHandler
{
public:
    explicit SpecialDaysHandler(
        std::shared_ptr<services::ISpecialDayService> specialDayService
    );

    /**
     * @brief Получает список особых дней с пагинацией и фильтрацией.
     * GET /special-days
     */
    void handleGetSpecialDays(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает особый день по ID.
     * GET /special-days/{id}
     */
    void handleGetSpecialDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новый особый день.
     * POST /special-days
     */
    void handleCreateSpecialDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет особый день.
     * PUT /special-days/{id}
     */
    void handleUpdateSpecialDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет особый день.
     * DELETE /special-days/{id}
     */
    void handleDeleteSpecialDay(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::ISpecialDayService> m_specialDayService;
};

} // namespace handlers
} // namespace server
