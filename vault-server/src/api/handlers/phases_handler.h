#pragma once

#include <memory>
#include <map>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iphase_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с фазами (CRUD операции).
 */
class PhasesHandler final : public BaseHandler
{
public:
    /**
     * @brief Конструктор обработчика.
     * @param phaseService Указатель на сервис фаз (содержит логику проверки прав)
     */
    explicit PhasesHandler(std::shared_ptr<services::IPhaseService> phaseService);

    /**
     * @brief Обрабатывает запрос на получение списка фаз.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleGetPhases(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на получение конкретной фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleGetPhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на создание новой фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleCreatePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на обновление существующей фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleUpdatePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    /**
     * @brief Обрабатывает запрос на архивацию (удаление) фазы.
     * @param request HTTP-запрос
     * @param userId ID аутентифицированного пользователя
     */
    void handleDeletePhase(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IPhaseService> m_phaseService;
};

} // namespace handlers
} // namespace server
