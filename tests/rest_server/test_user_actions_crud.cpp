#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_action_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct UserActionsTestFixture
{
    UserActionsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockUserActionService = std::make_shared<MockUserActionService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultUserActionService();

        server = std::make_unique<RestServer>("127.0.0.1", 18121);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setUserActionService(mockUserActionService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultUserActionService()
    {
        // Настройка списка действий
        services::UserActionsPage testPage;

        dto::UserAction action1 = MockUserActionService::createTestAction(
            1, 100, "Вход в систему", "Пользователь успешно вошёл"
        );
        dto::UserAction action2 = MockUserActionService::createTestAction(
            2, 100, "Создание проекта", "Создан проект 'Тестовый проект'"
        );
        dto::UserAction action3 = MockUserActionService::createTestAction(
            3, 100, "Обновление задачи", "Задача #42 обновлена"
        );

        testPage.actions = { action1, action2, action3 };
        testPage.totalCount = 3;
        mockUserActionService->setGetActionsResult(testPage);
        mockUserActionService->setGetActionResult(action1);

        dto::UserAction newAction = MockUserActionService::createTestAction(
            100, 100, "Новое действие", "Описание нового действия"
        );
        mockUserActionService->setCreateActionResult(newAction);

        services::UserActionResult deleteResult;
        deleteResult.success = true;
        mockUserActionService->setDeleteActionResult(deleteResult);
    }

    ~UserActionsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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

    pplx::task<web::http::http_response> makeDeleteRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
    std::shared_ptr<MockUserActionService> mockUserActionService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UserActionsCrudTestSuite, UserActionsTestFixture)

// ============================================================
// GET /api/v1/user-actions — Получение списка действий
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_actions_returns_list)
{
    auto response = makeGetRequest("/api/v1/user-actions").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserActionService->getGetActionsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastGetActionsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_actions_with_pagination_params)
{
    services::UserActionsPage emptyPage;
    mockUserActionService->setGetActionsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/user-actions?page=2&pageSize=10").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastGetActionsPage(), 2);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastGetActionsPageSize(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_actions_filter_by_user)
{
    services::UserActionsPage filteredPage;
    dto::UserAction action = MockUserActionService::createTestAction(
        42, 50, "Действие другого пользователя"
    );
    filteredPage.actions = { action };
    filteredPage.totalCount = 1;
    mockUserActionService->setGetActionsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-actions?userId=50").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserActionService->getLastGetActionsFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserActionService->getLastGetActionsFilterUserId(), 50);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 50);
}

BOOST_AUTO_TEST_CASE(test_get_actions_filter_by_date_range)
{
    services::UserActionsPage filteredPage;
    dto::UserAction action = MockUserActionService::createTestAction(
        42, 100, "Действие в диапазоне"
    );
    filteredPage.actions = { action };
    filteredPage.totalCount = 1;
    mockUserActionService->setGetActionsResult(filteredPage);

    auto response = makeGetRequest(
                        "/api/v1/user-actions?dateFrom=1700000000&dateTo=1800000000"
    )
                        .get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK(mockUserActionService->getLastGetActionsDateFrom().has_value());
    BOOST_CHECK(mockUserActionService->getLastGetActionsDateTo().has_value());
}

BOOST_AUTO_TEST_CASE(test_get_actions_requires_auth)
{
    auto response = makeGetRequest("/api/v1/user-actions", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserActionService->getGetActionsCallCount(), 0);
}

// ============================================================
// GET /api/v1/user-actions/{id} — Получение действия по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_action_by_id_success)
{
    dto::UserAction action = MockUserActionService::createTestAction(
        42, 100, "Конкретное действие", "Описание конкретного действия"
    );
    mockUserActionService->setGetActionResult(action);

    auto response = makeGetRequest("/api/v1/user-actions/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserActionService->getGetActionCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastGetActionId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретное действие"));
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_action_not_found)
{
    mockUserActionService->setGetActionResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/user-actions/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserActionService->getGetActionCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastGetActionId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_action_invalid_id)
{
    auto response = makeGetRequest("/api/v1/user-actions/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/user-actions — Создание действия
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_action_success)
{
    dto::UserAction createdAction = MockUserActionService::createTestAction(
        100, 100, "Новое действие", "Создано через API"
    );
    mockUserActionService->setCreateActionResult(createdAction);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новое действие"));
    body[U("description")] = web::json::value::string(U("Создано через API"));

    auto response = makePostRequest("/api/v1/user-actions", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserActionService->getCreateActionCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastCreateActionUserId(), 100);
    BOOST_CHECK_EQUAL(*mockUserActionService->getLastCreateAction().caption, "Новое действие");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новое действие"));
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_create_action_missing_required_fields)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Без заголовка"));

    auto response = makePostRequest("/api/v1/user-actions", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserActionService->getCreateActionCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_action_empty_caption)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U(""));

    auto response = makePostRequest("/api/v1/user-actions", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserActionService->getCreateActionCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_action_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockUserActionService->setCreateActionResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка создания"));
    body[U("userId")] = web::json::value::number(100);

    auto response = makePostRequest("/api/v1/user-actions", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/user-actions/{id} — Удаление действия
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_action_success)
{
    services::UserActionResult deleteResult;
    deleteResult.success = true;
    mockUserActionService->setDeleteActionResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-actions/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserActionService->getDeleteActionCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastDeleteActionId(), 3);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastDeleteActionUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_action_not_found)
{
    services::UserActionResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Action not found";
    mockUserActionService->setDeleteActionResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-actions/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserActionService->getDeleteActionCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserActionService->getLastDeleteActionId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_action_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::UserActionResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockUserActionService->setDeleteActionResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-actions/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_action_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/user-actions/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserActionService->getDeleteActionCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_action_lifecycle)
{
    // 1. Создание действия
    dto::UserAction newAction = MockUserActionService::createTestAction(
        200, 100, "Жизненный цикл действия", "Полный цикл тестирования"
    );
    mockUserActionService->setCreateActionResult(newAction);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл действия"));
    createBody[U("description")] = web::json::value::string(U("Полный цикл тестирования"));

    auto createResponse = makePostRequest("/api/v1/user-actions", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newActionId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newActionId, 0);

    // 2. Чтение созданного действия
    mockUserActionService->setGetActionResult(newAction);
    auto getResponse = makeGetRequest("/api/v1/user-actions/" + std::to_string(newActionId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение списка действий
    services::UserActionsPage listPage;
    listPage.actions = { newAction };
    listPage.totalCount = 1;
    mockUserActionService->setGetActionsResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/user-actions").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 4. Удаление действия
    services::UserActionResult deleteResult;
    deleteResult.success = true;
    mockUserActionService->setDeleteActionResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/user-actions/" + std::to_string(newActionId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockUserActionService->setGetActionResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/user-actions/" + std::to_string(newActionId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
