#include <regex>
#include <string>

#include <cpprest/uri.h>

#include "common/dto/activate_plan_request.h"
#include "common/dto/fork_plan_request.h"
#include "common/dto/plan.h"
#include "common/dto/plan_item.h"
#include "common/helpers/json_helper.hpp"
#include "common/log/log.h"

#include "plans_handler.h"

namespace server
{
namespace handlers
{

PlansHandler::PlansHandler(std::shared_ptr<services::IPlanService> planService)
    : m_planService(std::move(planService))
{
    if (!m_planService)
    {
        LOG_WARN << "PlansHandler инициализирован без PlanService";
    }
}

// ============================================================
// GET /phases/{phaseId}/plans
// ============================================================

void PlansHandler::handleGetPlansByPhase(
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

    // Извлекаем phaseId из пути: /phases/{phaseId}/plans
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/phases/(\d+)/plans)");
    std::smatch matches;

    int64_t phaseId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            phaseId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid phase ID");
            request.reply(resp);
            return;
        }
    }

    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    auto params = extractQueryParams(request);

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
            LOG_WARN << "handleGetPlansByPhase: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetPlansByPhase: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    std::optional<bool> isActive = std::nullopt;
    if (params.count("isActive"))
    {
        isActive = parseBool(params["isActive"]);
    }

    LOG_DEBUG
        << "GET /phases/" << phaseId << "/plans: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize
        << ", isActive=" << (isActive.has_value() ? (*isActive ? "true" : "false") : "none");

    try
    {
        auto plansPage = m_planService->plans(
            page, pageSize, userId, phaseId, isActive
        );

        web::json::value response;
        web::json::value items = web::json::value::array();

        for (size_t i = 0; i < plansPage.plans.size(); ++i)
        {
            items[i] = dto::toWebJson(plansPage.plans[i].toJson());
        }

        response["items"] = items;
        response["totalCount"] = web::json::value::number(plansPage.totalCount);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении списка планов: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /phases/{phaseId}/plans
// ============================================================

void PlansHandler::handleCreateFirstPlan(
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

    // Извлекаем phaseId из пути
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/phases/(\d+)/plans)");
    std::smatch matches;

    int64_t phaseId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            phaseId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid phase ID");
            request.reply(resp);
            return;
        }
    }

    if (phaseId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid phase ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /phases/" << phaseId << "/plans from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, phaseId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();

                    std::string caption;
                    if (jsonBody.has_field("caption"))
                    {
                        caption = utility::conversions::to_utf8string(
                            jsonBody.at("caption").as_string()
                        );
                    }
                    else
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    std::string description;
                    if (jsonBody.has_field("description"))
                    {
                        description = utility::conversions::to_utf8string(
                            jsonBody.at("description").as_string()
                        );
                    }

                    // Создаём план через специальный метод создания первого плана
                    dto::Plan plan;
                    plan.phaseId = phaseId;
                    plan.caption = caption;
                    plan.description = description;
                    plan.isActive = true; // Первый план становится активным

                    auto created = m_planService->createPlan(plan, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot create plan: phase already has a plan or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал первый план id=" << *created->id
                        << " в фазе " << phaseId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при создании плана: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// GET /plans/{id}
// ============================================================

void PlansHandler::handleGetPlan(
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

    const int64_t planId = extractIdFromPath(request);
    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /plans/" << planId << " from user " << userId;

    try
    {
        auto plan = m_planService->plan(planId, userId);
        if (!plan)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Plan not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(plan->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении плана " << planId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// DELETE /plans/{id}
// ============================================================

void PlansHandler::handleDeletePlan(
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

    const int64_t planId = extractIdFromPath(request);
    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /plans/" << planId << " from user " << userId;

    try
    {
        auto result = m_planService->deletePlan(planId, userId);
        if (!result.success)
        {
            web::http::http_response resp(
                static_cast<web::http::status_code>(result.errorCode)
            );
            sendErrorResponse(resp, result.errorCode, result.errorMessage);
            request.reply(resp);
            return;
        }

        LOG_INFO << "Пользователь " << userId << " удалил план " << planId;
        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении плана " << planId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /plans/{id}/fork
// ============================================================

void PlansHandler::handleForkPlan(
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

    const int64_t planId = extractIdFromPath(request);
    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /plans/" << planId << "/fork from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, planId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ForkPlanRequest forkRequest(nlohmannJson);

                    if (!forkRequest.caption.has_value() || forkRequest.caption->empty())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "Caption is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_planService->forkPlan(
                        planId,
                        *forkRequest.caption,
                        forkRequest.description.value_or(""),
                        userId
                    );

                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot fork plan: insufficient permissions or plan is not active"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " создал форк плана " << planId
                        << ", новый план id=" << *created->id;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при форке плана: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// POST /plans/{id}/activate
// ============================================================

void PlansHandler::handleActivatePlan(
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

    const int64_t planId = extractIdFromPath(request);
    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /plans/" << planId << "/activate from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, planId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    dto::ActivatePlanRequest activateRequest(nlohmannJson);

                    if (!activateRequest.activatedByUserId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "activatedByUserId is required");
                        request.reply(resp);
                        return;
                    }

                    // Проверяем, что активирует тот же пользователь
                    if (*activateRequest.activatedByUserId != userId)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(resp, 403, "You can only activate plan for yourself");
                        request.reply(resp);
                        return;
                    }

                    auto result = m_planService->activatePlan(planId, userId);
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
                        << " активировал план " << planId;

                    request.reply(web::http::status_codes::OK);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при активации плана: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// GET /plans/{planId}/items
// ============================================================

void PlansHandler::handleGetPlanItems(
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

    // Извлекаем planId из пути: /plans/{planId}/items
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/plans/(\d+)/items)");
    std::smatch matches;

    int64_t planId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            planId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid plan ID");
            request.reply(resp);
            return;
        }
    }

    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    auto params = extractQueryParams(request);

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
            LOG_WARN << "handleGetPlanItems: неверный параметр page: " << params["page"];
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
            LOG_WARN << "handleGetPlanItems: неверный параметр pageSize: " << params["pageSize"];
        }
    }

    std::optional<int64_t> userIdFilter = std::nullopt;
    if (params.count("userId"))
    {
        try
        {
            userIdFilter = std::stoll(params["userId"]);
        }
        catch (const std::exception& e)
        {
            LOG_WARN << "handleGetPlanItems: неверный параметр userId: " << params["userId"];
        }
    }

    LOG_DEBUG
        << "GET /plans/" << planId << "/items: user=" << userId
        << ", page=" << page << ", pageSize=" << pageSize;

    try
    {
        // Используем findAll для пагинации
        auto [items, total] = m_planService->getPlanItemsWithPagination(
            page, pageSize, planId, userIdFilter, userId
        );

        web::json::value response;
        web::json::value itemsArray = web::json::value::array();

        for (size_t i = 0; i < items.size(); ++i)
        {
            itemsArray[i] = dto::toWebJson(items[i].toJson());
        }

        response["items"] = itemsArray;
        response["totalCount"] = web::json::value::number(total);
        response["page"] = web::json::value::number(page);
        response["pageSize"] = web::json::value::number(pageSize);

        request.reply(web::http::status_codes::OK, response);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элементов плана: " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// POST /plans/{planId}/items
// ============================================================

void PlansHandler::handleAddPlanItem(
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

    // Извлекаем planId из пути
    std::string path = web::uri::decode(request.relative_uri().path());
    static const std::regex pattern(R"(/plans/(\d+)/items)");
    std::smatch matches;

    int64_t planId = -1;
    if (std::regex_search(path, matches, pattern) && matches.size() > 1)
    {
        try
        {
            planId = std::stoll(matches[1].str());
        }
        catch (const std::exception& e)
        {
            web::http::http_response resp(web::http::status_codes::BadRequest);
            sendErrorResponse(resp, 400, "Invalid plan ID");
            request.reply(resp);
            return;
        }
    }

    if (planId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "POST /plans/" << planId << "/items from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, planId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);

                    dto::PlanItem planItem(nlohmannJson);
                    planItem.planId = planId;

                    // Валидация
                    if (!planItem.itemId.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "itemId is required");
                        request.reply(resp);
                        return;
                    }

                    if (!planItem.startDate.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "startDate is required");
                        request.reply(resp);
                        return;
                    }

                    if (!planItem.endDate.has_value())
                    {
                        web::http::http_response resp(web::http::status_codes::BadRequest);
                        sendErrorResponse(resp, 400, "endDate is required");
                        request.reply(resp);
                        return;
                    }

                    auto created = m_planService->addPlanItem(planItem, userId);
                    if (!created)
                    {
                        web::http::http_response resp(web::http::status_codes::Forbidden);
                        sendErrorResponse(
                            resp,
                            403,
                            "Cannot add item to plan: plan is active, item already in plan, or dates invalid"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " добавил элемент " << *planItem.itemId
                        << " в план " << planId;

                    request.reply(
                        web::http::status_codes::Created,
                        dto::toWebJson(created->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при добавлении элемента в план: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// GET /plan-items/{id}
// ============================================================

void PlansHandler::handleGetPlanItem(
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

    const int64_t planItemId = extractIdFromPath(request);
    if (planItemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "GET /plan-items/" << planItemId << " from user " << userId;

    try
    {
        // Получаем элемент плана через план
        auto planItem = m_planService->getPlanItem(planItemId, userId);
        if (!planItem)
        {
            web::http::http_response resp(web::http::status_codes::NotFound);
            sendErrorResponse(resp, 404, "Plan item not found");
            request.reply(resp);
            return;
        }

        request.reply(
            web::http::status_codes::OK,
            dto::toWebJson(planItem->toJson())
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при получении элемента плана " << planItemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

// ============================================================
// PUT /plan-items/{id}
// ============================================================

void PlansHandler::handleUpdatePlanItem(
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

    const int64_t planItemId = extractIdFromPath(request);
    if (planItemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "PUT /plan-items/" << planItemId << " from user " << userId;

    request
        .extract_json()
        .then(
            [this, request, userId, planItemId](pplx::task<web::json::value> task)
            {
                try
                {
                    auto jsonBody = task.get();
                    auto nlohmannJson = dto::toNlohmannJson(jsonBody);
                    nlohmannJson["id"] = planItemId;

                    dto::PlanItem planItem(nlohmannJson);

                    auto updated = m_planService->updatePlanItem(planItem, userId);
                    if (!updated)
                    {
                        web::http::http_response resp(web::http::status_codes::NotFound);
                        sendErrorResponse(
                            resp,
                            404,
                            "Plan item not found or insufficient permissions"
                        );
                        request.reply(resp);
                        return;
                    }

                    LOG_INFO
                        << "Пользователь " << userId
                        << " обновил элемент плана " << planItemId;

                    request.reply(
                        web::http::status_codes::OK,
                        dto::toWebJson(updated->toJson())
                    );
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR << "Ошибка при обновлении элемента плана: " << e.what();
                    web::http::http_response resp(web::http::status_codes::BadRequest);
                    sendErrorResponse(resp, 400, std::string("Invalid request: ") + e.what());
                    request.reply(resp);
                }
            }
        )
        .wait();
}

// ============================================================
// DELETE /plan-items/{id}
// ============================================================

void PlansHandler::handleDeletePlanItem(
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

    const int64_t planItemId = extractIdFromPath(request);
    if (planItemId <= 0)
    {
        web::http::http_response resp(web::http::status_codes::BadRequest);
        sendErrorResponse(resp, 400, "Invalid plan item ID");
        request.reply(resp);
        return;
    }

    LOG_DEBUG << "DELETE /plan-items/" << planItemId << " from user " << userId;

    try
    {
        auto result = m_planService->removePlanItem(planItemId, userId);
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
            << " удалил элемент плана " << planItemId;

        request.reply(web::http::status_codes::NoContent);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Ошибка при удалении элемента плана " << planItemId << ": " << e.what();
        web::http::http_response resp(web::http::status_codes::InternalError);
        sendErrorResponse(resp, 500, "Internal server error");
        request.reply(resp);
    }
}

} // namespace handlers
} // namespace server
