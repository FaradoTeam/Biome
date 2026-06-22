#pragma once

#include <memory>
#include <string>

#include <cpprest/http_msg.h>
#include <cpprest/json.h>

#include "logic/iplan_service.h"

#include "base_handler.h"

namespace server
{
namespace handlers
{

/**
 * @brief Обработчик запросов для работы с планами и их элементами.
 */
class PlansHandler final : public BaseHandler
{
public:
    explicit PlansHandler(std::shared_ptr<services::IPlanService> planService);

    // ============================================================
    // GET /phases/{phaseId}/plans - список планов фазы
    // ============================================================
    void handleGetPlansByPhase(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // POST /phases/{phaseId}/plans - создание первого плана в фазе
    // ============================================================
    void handleCreateFirstPlan(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // GET /plans/{id} - получение плана по ID
    // ============================================================
    void handleGetPlan(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // DELETE /plans/{id} - удаление плана (только черновик)
    // ============================================================
    void handleDeletePlan(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // POST /plans/{id}/fork - создание новой версии плана
    // ============================================================
    void handleForkPlan(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // POST /plans/{id}/activate - активация плана
    // ============================================================
    void handleActivatePlan(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // GET /plans/{planId}/items - получение элементов плана
    // ============================================================
    void handleGetPlanItems(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // POST /plans/{planId}/items - добавление элемента в план
    // ============================================================
    void handleAddPlanItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // GET /plan-items/{id} - получение элемента плана по ID
    // ============================================================
    void handleGetPlanItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // PUT /plan-items/{id} - обновление элемента плана
    // ============================================================
    void handleUpdatePlanItem(
        const web::http::http_request& request,
        const std::string& userId
    );

    // ============================================================
    // DELETE /plan-items/{id} - удаление элемента из плана
    // ============================================================
    void handleDeletePlanItem(
        const web::http::http_request& request,
        const std::string& userId
    );

private:
    std::shared_ptr<services::IPlanService> m_planService;
};

} // namespace handlers
} // namespace server
