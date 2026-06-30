#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iuser_day_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с пользовательскими днями.
 */
class UserDaysHandler final : public BaseHandler
{
public:
    explicit UserDaysHandler(
        std::shared_ptr<services::IUserDayService> userDayService
    );

    /**
     * @brief Получает список пользовательских дней с пагинацией и фильтрацией.
     * GET /user-days
     */
    void handleGetUserDays(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает пользовательский день по ID.
     * GET /user-days/{id}
     */
    void handleGetUserDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Получает пользовательский день по пользователю и дате.
     * GET /users/{userId}/days/{date}
     */
    void handleGetUserDayByUserAndDate(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Создаёт новый пользовательский день.
     * POST /user-days
     */
    void handleCreateUserDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обновляет пользовательский день.
     * PUT /user-days/{id}
     */
    void handleUpdateUserDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет пользовательский день.
     * DELETE /user-days/{id}
     */
    void handleDeleteUserDay(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Удаляет все пользовательские дни для указанного пользователя.
     * DELETE /users/{userId}/days
     */
    void handleDeleteUserDaysByUser(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IUserDayService> m_userDayService;
};

} // namespace handlers
} // namespace server
