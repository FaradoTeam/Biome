#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "common/types.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_phase_service.h"
#include "tests/server_mocks/mock_plan_service.h"
#include "tests/server_mocks/mock_project_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct PlansTestFixture
{
    PlansTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockProjectService = std::make_shared<MockProjectService>();
        mockPhaseService = std::make_shared<MockPhaseService>();
        mockPlanService = std::make_shared<server::tests::MockPlanService>();

        // Супер-админ (userId=1) для создания/обновления/удаления планов
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultPlanService();

        server = std::make_unique<RestServer>("127.0.0.1", 18120);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setProjectService(mockProjectService);
        server->setPhaseService(mockPhaseService);
        server->setPlanService(mockPlanService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultPlanService()
    {
        // Настройка для теста создания первого плана - фаза 10 не должна иметь планов
        services::PlansPage emptyPlansPage;
        emptyPlansPage.totalCount = 0;
        mockPlanService->setGetPlansResultForPhase(10, emptyPlansPage);

        // Настройка списка планов для фазы 10 (для тестов GET)
        services::PlansPage testPage;

        dto::Plan plan1;
        plan1.id = 1;
        plan1.phaseId = 10;
        plan1.caption = "План 1";
        plan1.description = "Описание плана 1";
        plan1.isActive = false;
        plan1.createdByUserId = 1;

        dto::Plan plan2;
        plan2.id = 2;
        plan2.phaseId = 10;
        plan2.caption = "План 2";
        plan2.description = "Описание плана 2";
        plan2.isActive = true;
        plan2.createdByUserId = 1;
        plan2.activatedByUserId = 1;

        dto::Plan plan3;
        plan3.id = 3;
        plan3.phaseId = 10;
        plan3.caption = "Архивный план";
        plan3.description = "Описание архивного плана";
        plan3.isActive = false;
        plan3.createdByUserId = 1;

        testPage.plans = { plan1, plan2, plan3 };
        testPage.totalCount = 3;

        // Устанавливаем результат для фазы 10
        mockPlanService->setGetPlansResultForPhase(10, testPage);
        // Также устанавливаем общий результат (для тестов без фильтрации)
        mockPlanService->setGetPlansResult(testPage);
        mockPlanService->setGetPlanResult(plan1);

        // Настраиваем callback для createFirstPlan
        mockPlanService->setCreateFirstPlanCallback(
            [this](int64_t phaseId, const std::string& caption, const std::string& description, int64_t userId)
                -> std::optional<dto::Plan>
            {
                // Только супер-админ может создавать планы
                if (userId != 1)
                {
                    return std::nullopt;
                }

                dto::Plan newPlan;
                newPlan.id = 100;
                newPlan.phaseId = phaseId;
                newPlan.caption = caption;
                newPlan.description = description;
                newPlan.isActive = true;
                newPlan.createdByUserId = userId;
                return newPlan;
            }
        );

        // Настраиваем callback для forkPlan
        mockPlanService->setForkPlanCallback(
            [this](int64_t planId, const std::string& caption, const std::string& description, int64_t userId)
                -> std::optional<dto::Plan>
            {
                if (userId != 1)
                {
                    return std::nullopt;
                }
                dto::Plan forkedPlan;
                forkedPlan.id = 101;
                forkedPlan.phaseId = 10;
                forkedPlan.basePlanId = planId;
                forkedPlan.caption = caption;
                forkedPlan.description = description;
                forkedPlan.isActive = false;
                forkedPlan.createdByUserId = userId;
                return forkedPlan;
            }
        );

        // Настраиваем callback для activatePlan
        mockPlanService->setActivatePlanCallback(
            [this](int64_t id, int64_t activatedByUserId) -> services::PlanResult
            {
                services::PlanResult result;
                if (activatedByUserId != 1)
                {
                    result.success = false;
                    result.errorCode = 403;
                    result.errorMessage = "Insufficient permissions";
                    return result;
                }
                result.success = true;
                return result;
            }
        );

        // Настраиваем callback для deletePlan
        mockPlanService->setDeletePlanCallback(
            [this](int64_t id, int64_t userId) -> services::PlanResult
            {
                services::PlanResult result;
                if (userId != 1)
                {
                    result.success = false;
                    result.errorCode = 403;
                    result.errorMessage = "Insufficient permissions";
                    return result;
                }
                result.success = true;
                return result;
            }
        );

        // Настраиваем callback для addPlanItem
        mockPlanService->setAddPlanItemCallback(
            [this](const dto::PlanItem& planItem, int64_t userId) -> std::optional<dto::PlanItem>
            {
                if (userId != 1)
                {
                    return std::nullopt;
                }
                dto::PlanItem newItem = planItem;
                newItem.id = 100;
                return newItem;
            }
        );

        // Настраиваем callback для updatePlanItem
        mockPlanService->setUpdatePlanItemCallback(
            [this](const dto::PlanItem& planItem, int64_t userId) -> std::optional<dto::PlanItem>
            {
                if (userId != 1)
                {
                    return std::nullopt;
                }
                return planItem;
            }
        );

        // Настраиваем callback для removePlanItem
        mockPlanService->setRemovePlanItemCallback(
            [this](int64_t planItemId, int64_t userId) -> services::PlanResult
            {
                services::PlanResult result;
                if (userId != 1)
                {
                    result.success = false;
                    result.errorCode = 403;
                    result.errorMessage = "Insufficient permissions";
                    return result;
                }
                result.success = true;
                return result;
            }
        );

        // Настройка элементов плана
        auto now = std::chrono::system_clock::now();

        dto::PlanItem item1;
        item1.id = 1;
        item1.planId = 2;
        item1.itemId = 100;
        item1.userId = 1;
        item1.startDate = now;
        item1.endDate = now + std::chrono::hours(24 * 5);

        dto::PlanItem item2;
        item2.id = 2;
        item2.planId = 2;
        item2.itemId = 101;
        item2.userId = 2;
        item2.startDate = now + std::chrono::hours(24);
        item2.endDate = now + std::chrono::hours(24 * 10);

        services::PlanItemsPage itemsPage;
        itemsPage.items = { item1, item2 };
        itemsPage.totalCount = 2;
        mockPlanService->setGetPlanItemsResult(itemsPage);
        mockPlanService->setGetPlanItemResult(item1);
    }

    ~PlansTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<web::http::http_response> makeGetRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
        web::http::http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        return client.request(request);
    }

    pplx::task<web::http::http_response> makePostRequest(
        const std::string& path,
        const web::json::value& body,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
        web::http::http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        request.set_body(body);
        request.headers().set_content_type(U("application/json"));
        return client.request(request);
    }

    pplx::task<web::http::http_response> makePutRequest(
        const std::string& path,
        const web::json::value& body,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
        web::http::http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        request.set_body(body);
        request.headers().set_content_type(U("application/json"));
        return client.request(request);
    }

    pplx::task<web::http::http_response> makeDeleteRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
        web::http::http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockUserService> mockUserService;
    std::shared_ptr<MockProjectService> mockProjectService;
    std::shared_ptr<MockPhaseService> mockPhaseService;
    std::shared_ptr<server::tests::MockPlanService> mockPlanService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(PlansCrudTestSuite, PlansTestFixture)

// ============================================================
// GET /api/v1/phases/{phaseId}/plans — Получение списка планов фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_plans_by_phase_returns_list)
{
    auto response = makeGetRequest("/api/v1/phases/10/plans").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlansCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlansUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_plans_by_phase_with_pagination)
{
    services::PlansPage emptyPage;
    mockPlanService->setGetPlansResult(emptyPage);

    auto response = makeGetRequest("/api/v1/phases/10/plans?page=2&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlansPage(), 2);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlansPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_plans_by_phase_filter_by_active)
{
    // Создаём отфильтрованный результат с одним активным планом
    services::PlansPage filteredPage;
    dto::Plan activePlan;
    activePlan.id = 2;
    activePlan.phaseId = 10;
    activePlan.caption = "Активный план";
    activePlan.isActive = true;
    filteredPage.plans = { activePlan };
    filteredPage.totalCount = 1;

    // Устанавливаем callback, который фильтрует по isActive
    mockPlanService->setGetPlansCallback(
        [filteredPage](int page, int pageSize, int64_t userId, std::optional<int64_t> phaseId, std::optional<bool> isActive)
            -> services::PlansPage
        {
            if (isActive.has_value() && isActive.value())
            {
                return filteredPage;
            }
            return filteredPage;
        }
    );

    auto response = makeGetRequest("/api/v1/phases/10/plans?isActive=true").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPlanService->getLastGetPlansIsActive().has_value());
    BOOST_CHECK(mockPlanService->getLastGetPlansIsActive().value());

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK(json.at(U("items"))[0].at(U("isActive")).as_bool());
}

BOOST_AUTO_TEST_CASE(test_get_plans_by_phase_invalid_phase_id)
{
    auto response = makeGetRequest("/api/v1/phases/invalid/plans").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_plans_by_phase_requires_auth)
{
    auto response = makeGetRequest("/api/v1/phases/10/plans", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlansCallCount(), 0);
}

// ============================================================
// POST /api/v1/phases/{phaseId}/plans — Создание первого плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_first_plan_success)
{
    // Убеждаемся, что фаза 10 не имеет планов
    services::PlansPage emptyPlansPage;
    emptyPlansPage.totalCount = 0;
    mockPlanService->setGetPlansResultForPhase(10, emptyPlansPage);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый план"));
    body[U("description")] = web::json::value::string(U("Описание нового плана"));

    auto response = makePostRequest("/api/v1/phases/10/plans", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockPlanService->getCreateFirstPlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastCreateFirstPlanUserId(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastCreateFirstPlanCaption(), "Новый план");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новый план"));
}

BOOST_AUTO_TEST_CASE(test_create_first_plan_missing_caption)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Без названия"));

    auto response = makePostRequest("/api/v1/phases/10/plans", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPlanService->getCreateFirstPlanCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_first_plan_plan_already_exists)
{
    // Устанавливаем что фаза 10 уже имеет план
    services::PlansPage existingPage;
    dto::Plan existingPlan;
    existingPlan.id = 1;
    existingPlan.phaseId = 10;
    existingPlan.caption = "Существующий план";
    existingPlan.isActive = true;
    existingPage.plans = { existingPlan };
    existingPage.totalCount = 1;
    mockPlanService->setGetPlansResultForPhase(10, existingPage);

    // Создаём callback, который проверяет наличие плана в фазе
    mockPlanService->setCreateFirstPlanCallback(
        [this](int64_t phaseId, const std::string& caption, const std::string& description, int64_t userId)
            -> std::optional<dto::Plan>
        {
            if (userId != 1)
            {
                return std::nullopt;
            }

            // Проверяем, есть ли планы в фазе через вызов plans
            auto plansPage = mockPlanService->plans(1, 1, userId, phaseId);
            if (plansPage.totalCount > 0)
            {
                return std::nullopt; // План уже существует
            }

            dto::Plan newPlan;
            newPlan.id = 100;
            newPlan.phaseId = phaseId;
            newPlan.caption = caption;
            newPlan.description = description;
            newPlan.isActive = true;
            newPlan.createdByUserId = userId;
            return newPlan;
        }
    );

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("План"));

    auto response = makePostRequest("/api/v1/phases/10/plans", body).get();

    // TODO : Должен вернуть 400 Bad Request, так как план уже существует
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// GET /api/v1/plans/{id} — Получение плана по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_plan_by_id_success)
{
    dto::Plan plan;
    plan.id = 42;
    plan.phaseId = 10;
    plan.caption = "Конкретный план";
    plan.description = "Описание плана";
    plan.isActive = false;
    plan.createdByUserId = 1;
    mockPlanService->setGetPlanResult(plan);

    auto response = makeGetRequest("/api/v1/plans/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanId(), 42);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретный план"));
}

BOOST_AUTO_TEST_CASE(test_get_plan_not_found)
{
    mockPlanService->setGetPlanResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/plans/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_plan_invalid_id)
{
    auto response = makeGetRequest("/api/v1/plans/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/plans/{id} — Удаление плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_plan_success)
{
    auto response = makeDeleteRequest("/api/v1/plans/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockPlanService->getDeletePlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastDeletedPlanId(), 3);
    BOOST_CHECK_EQUAL(mockPlanService->getLastDeletePlanUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_active_fails)
{
    // Настраиваем deletePlanCallback для возврата ошибки для активного плана
    mockPlanService->setDeletePlanCallback(
        [](int64_t id, int64_t userId) -> services::PlanResult
        {
            services::PlanResult result;
            if (id == 2) // План 2 активен
            {
                result.success = false;
                result.errorCode = 400;
                result.errorMessage = "Cannot delete active plan";
            }
            else
            {
                result.success = true;
            }
            return result;
        }
    );

    auto response = makeDeleteRequest("/api/v1/plans/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_not_found)
{
    // Настраиваем deletePlanCallback для возврата 404
    mockPlanService->setDeletePlanCallback(
        [](int64_t id, int64_t userId) -> services::PlanResult
        {
            services::PlanResult result;
            if (id == 999)
            {
                result.success = false;
                result.errorCode = 404;
                result.errorMessage = "Plan not found";
            }
            else
            {
                result.success = true;
            }
            return result;
        }
    );

    auto response = makeDeleteRequest("/api/v1/plans/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/plans/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPlanService->getDeletePlanCallCount(), 0);
}

// ============================================================
// POST /api/v1/plans/{id}/fork — Создание форка плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_fork_plan_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Форк плана"));
    body[U("description")] = web::json::value::string(U("Описание форка"));

    auto response = makePostRequest("/api/v1/plans/2/fork", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockPlanService->getForkPlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastForkPlanId(), 2);
    BOOST_CHECK_EQUAL(mockPlanService->getLastForkPlanUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 101);
    BOOST_CHECK_EQUAL(json.at(U("basePlanId")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Форк плана"));
}

BOOST_AUTO_TEST_CASE(test_fork_plan_missing_caption)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Без названия"));

    auto response = makePostRequest("/api/v1/plans/2/fork", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPlanService->getForkPlanCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_fork_plan_not_active)
{
    // Настраиваем forkPlanCallback для возврата ошибки для неактивного плана
    mockPlanService->setForkPlanCallback(
        [](int64_t planId, const std::string& caption, const std::string& description, int64_t userId) -> std::optional<dto::Plan>
        {
            if (planId == 1) // План 1 неактивен
            {
                return std::nullopt;
            }
            dto::Plan forkedPlan;
            forkedPlan.id = 101;
            forkedPlan.phaseId = 10;
            forkedPlan.basePlanId = planId;
            forkedPlan.caption = caption;
            forkedPlan.description = description;
            forkedPlan.isActive = false;
            forkedPlan.createdByUserId = userId;
            return forkedPlan;
        }
    );

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Форк неактивного плана"));

    auto response = makePostRequest("/api/v1/plans/1/fork", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// POST /api/v1/plans/{id}/activate — Активация плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_activate_plan_success)
{
    web::json::value body;
    body[U("activatedByUserId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/plans/1/activate", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getActivatePlanCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastActivatePlanId(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastActivatePlanUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_activate_plan_missing_user_id)
{
    web::json::value body = web::json::value::object();

    auto response = makePostRequest("/api/v1/plans/1/activate", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPlanService->getActivatePlanCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_activate_plan_wrong_user)
{
    web::json::value body;
    body[U("activatedByUserId")] = web::json::value::number(200);

    auto response = makePostRequest("/api/v1/plans/1/activate", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_activate_plan_empty_plan)
{
    // Настраиваем activatePlanCallback для возврата ошибки
    mockPlanService->setActivatePlanCallback(
        [](int64_t id, int64_t activatedByUserId) -> services::PlanResult
        {
            services::PlanResult result;
            if (id == 3) // План 3 пустой
            {
                result.success = false;
                result.errorCode = 400;
                result.errorMessage = "Cannot activate empty plan";
            }
            else
            {
                result.success = true;
            }
            return result;
        }
    );

    web::json::value body;
    body[U("activatedByUserId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/plans/3/activate", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

// ============================================================
// GET /api/v1/plans/{planId}/items — Получение элементов плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_plan_items_returns_list)
{
    auto response = makeGetRequest("/api/v1/plans/2/items").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlanItemsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemsPlanId(), 2);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemsUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_plan_items_with_pagination)
{
    services::PlanItemsPage emptyPage;
    mockPlanService->setGetPlanItemsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/plans/2/items?page=2&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemsPage(), 2);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_plan_items_filter_by_user)
{
    services::PlanItemsPage filteredPage;
    dto::PlanItem item;
    item.id = 10;
    item.planId = 2;
    item.itemId = 100;
    item.userId = 200;
    filteredPage.items = { item };
    filteredPage.totalCount = 1;
    mockPlanService->setGetPlanItemsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/plans/2/items?userId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPlanService->getLastGetPlanItemsUserIdFilter().has_value());
    BOOST_CHECK_EQUAL(*mockPlanService->getLastGetPlanItemsUserIdFilter(), 200);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_plan_items_empty)
{
    services::PlanItemsPage emptyPage;
    mockPlanService->setGetPlanItemsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/plans/999/items").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

// ============================================================
// POST /api/v1/plans/{planId}/items — Добавление элемента в план
// ============================================================

BOOST_AUTO_TEST_CASE(test_add_plan_item_success)
{
    auto now = std::chrono::system_clock::now();

    web::json::value body;
    body[U("itemId")] = web::json::value::number(102);
    body[U("startDate")] = web::json::value::number(
        common::timePointToSeconds(now + std::chrono::hours(1))
    );
    body[U("endDate")] = web::json::value::number(
        common::timePointToSeconds(now + std::chrono::hours(24 * 3))
    );
    body[U("userId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/plans/1/items", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockPlanService->getAddPlanItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastAddPlanItemUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("planId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 102);
}

BOOST_AUTO_TEST_CASE(test_add_plan_item_missing_required_fields)
{
    web::json::value body;
    body[U("itemId")] = web::json::value::number(102);

    auto response = makePostRequest("/api/v1/plans/1/items", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPlanService->getAddPlanItemCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_add_plan_item_missing_item_id)
{
    auto now = std::chrono::system_clock::now();
    web::json::value body;
    body[U("startDate")] = web::json::value::number(common::timePointToSeconds(now));
    body[U("endDate")] = web::json::value::number(common::timePointToSeconds(now + std::chrono::hours(24)));

    auto response = makePostRequest("/api/v1/plans/1/items", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

BOOST_AUTO_TEST_CASE(test_add_plan_item_to_active_plan_fails)
{
    // Настраиваем addPlanItemCallback для возврата ошибки для активного плана
    mockPlanService->setAddPlanItemCallback(
        [](const dto::PlanItem& planItem, int64_t userId) -> std::optional<dto::PlanItem>
        {
            if (planItem.planId == 2) // План 2 активен
            {
                return std::nullopt;
            }
            return planItem;
        }
    );

    auto now = std::chrono::system_clock::now();
    web::json::value body;
    body[U("itemId")] = web::json::value::number(102);
    body[U("startDate")] = web::json::value::number(common::timePointToSeconds(now));
    body[U("endDate")] = web::json::value::number(common::timePointToSeconds(now + std::chrono::hours(24)));

    auto response = makePostRequest("/api/v1/plans/2/items", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// GET /api/v1/plan-items/{id} — Получение элемента плана по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_plan_item_by_id_success)
{
    dto::PlanItem item;
    item.id = 42;
    item.planId = 2;
    item.itemId = 100;
    item.userId = 1;
    mockPlanService->setGetPlanItemResult(item);

    auto response = makeGetRequest("/api/v1/plan-items/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getGetPlanItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemId(), 42);
    BOOST_CHECK_EQUAL(mockPlanService->getLastGetPlanItemUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("planId")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_plan_item_not_found)
{
    mockPlanService->setGetPlanItemResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/plan-items/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// PUT /api/v1/plan-items/{id} — Обновление элемента плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_plan_item_success)
{
    auto now = std::chrono::system_clock::now();

    web::json::value body;
    body[U("userId")] = web::json::value::number(3);
    body[U("endDate")] = web::json::value::number(
        common::timePointToSeconds(now + std::chrono::hours(24 * 7))
    );

    auto response = makePutRequest("/api/v1/plan-items/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getUpdatePlanItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastUpdatePlanItemUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 3);
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_partial)
{
    web::json::value body;
    body[U("userId")] = web::json::value::number(4);

    auto response = makePutRequest("/api/v1/plan-items/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPlanService->getUpdatePlanItemCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_not_found)
{
    // Настраиваем updatePlanItemCallback для возврата ошибки
    mockPlanService->setUpdatePlanItemCallback(
        [](const dto::PlanItem& planItem, int64_t userId) -> std::optional<dto::PlanItem>
        {
            if (planItem.id == 999)
            {
                return std::nullopt;
            }
            return planItem;
        }
    );

    web::json::value body;
    body[U("userId")] = web::json::value::number(1);

    auto response = makePutRequest("/api/v1/plan-items/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_update_plan_item_in_active_plan_fails)
{
    // Настраиваем updatePlanItemCallback для возврата ошибки для активного плана
    mockPlanService->setUpdatePlanItemCallback(
        [](const dto::PlanItem& planItem, int64_t userId) -> std::optional<dto::PlanItem>
        {
            if (planItem.id == 2) // Элемент в активном плане
            {
                return std::nullopt;
            }
            return planItem;
        }
    );

    web::json::value body;
    body[U("userId")] = web::json::value::number(3);

    auto response = makePutRequest("/api/v1/plan-items/2", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/plan-items/{id} — Удаление элемента из плана
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_plan_item_success)
{
    auto response = makeDeleteRequest("/api/v1/plan-items/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockPlanService->getRemovePlanItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPlanService->getLastRemovePlanItemId(), 3);
    BOOST_CHECK_EQUAL(mockPlanService->getLastRemovePlanItemUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_item_not_found)
{
    // Настраиваем removePlanItemCallback для возврата ошибки
    mockPlanService->setRemovePlanItemCallback(
        [](int64_t planItemId, int64_t userId) -> services::PlanResult
        {
            services::PlanResult result;
            if (planItemId == 999)
            {
                result.success = false;
                result.errorCode = 404;
                result.errorMessage = "Plan item not found";
            }
            else
            {
                result.success = true;
            }
            return result;
        }
    );

    auto response = makeDeleteRequest("/api/v1/plan-items/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_item_from_active_plan_fails)
{
    // Настраиваем removePlanItemCallback для возврата ошибки для активного плана
    mockPlanService->setRemovePlanItemCallback(
        [](int64_t planItemId, int64_t userId) -> services::PlanResult
        {
            services::PlanResult result;
            if (planItemId == 2) // Элемент в активном плане
            {
                result.success = false;
                result.errorCode = 400;
                result.errorMessage = "Cannot remove item from active plan";
            }
            else
            {
                result.success = true;
            }
            return result;
        }
    );

    auto response = makeDeleteRequest("/api/v1/plan-items/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

BOOST_AUTO_TEST_CASE(test_delete_plan_item_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/plan-items/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPlanService->getRemovePlanItemCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_plan_lifecycle)
{
    // Убеждаемся, что фаза 10 не имеет планов перед созданием первого
    services::PlansPage emptyPlansPage;
    emptyPlansPage.totalCount = 0;
    mockPlanService->setGetPlansResultForPhase(10, emptyPlansPage);

    // 1. Создание первого плана в фазе
    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл плана"));
    createBody[U("description")] = web::json::value::string(U("Тестовый план"));

    auto createResponse = makePostRequest("/api/v1/phases/10/plans", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newPlanId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newPlanId, 0);

    // 2. Чтение созданного плана
    auto getResponse = makeGetRequest("/api/v1/plans/" + std::to_string(newPlanId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Добавление элемента в план
    auto now = std::chrono::system_clock::now();
    web::json::value addItemBody;
    addItemBody[U("itemId")] = web::json::value::number(500);
    addItemBody[U("startDate")] = web::json::value::number(
        common::timePointToSeconds(now + std::chrono::hours(24))
    );
    addItemBody[U("endDate")] = web::json::value::number(
        common::timePointToSeconds(now + std::chrono::hours(24 * 5))
    );
    addItemBody[U("userId")] = web::json::value::number(1);

    auto addItemResponse = makePostRequest("/api/v1/plans/" + std::to_string(newPlanId) + "/items", addItemBody).get();
    BOOST_CHECK_EQUAL(addItemResponse.status_code(), status_codes::Created);

    // 4. Получение элементов плана
    auto getItemsResponse = makeGetRequest("/api/v1/plans/" + std::to_string(newPlanId) + "/items").get();
    BOOST_CHECK_EQUAL(getItemsResponse.status_code(), status_codes::OK);

    // 5. Обновление элемента плана
    web::json::value updateItemBody;
    updateItemBody[U("userId")] = web::json::value::number(2);

    auto updateItemResponse = makePutRequest("/api/v1/plan-items/200", updateItemBody).get();
    BOOST_CHECK_EQUAL(updateItemResponse.status_code(), status_codes::OK);

    // 6. Удаление элемента плана
    auto deleteItemResponse = makeDeleteRequest("/api/v1/plan-items/200").get();
    BOOST_CHECK_EQUAL(deleteItemResponse.status_code(), status_codes::NoContent);

    // 7. Форк плана
    web::json::value forkBody;
    forkBody[U("caption")] = web::json::value::string(U("Форк плана"));
    forkBody[U("description")] = web::json::value::string(U("Форк для теста"));

    auto forkResponse = makePostRequest("/api/v1/plans/" + std::to_string(newPlanId) + "/fork", forkBody).get();
    BOOST_CHECK_EQUAL(forkResponse.status_code(), status_codes::Created);

    // 8. Удаление плана (форк неактивен, можно удалить)
    auto deleteResponse = makeDeleteRequest("/api/v1/plans/201").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
