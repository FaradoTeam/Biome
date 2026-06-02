#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_state_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct StatesTestFixture
{
    StatesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockStateService = std::make_shared<MockStateService>();

        // Супер-админ (userId=1) для создания/обновления/удаления состояний
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18086);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setStateService(mockStateService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread([this]()
                                   { server->start(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~StatesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
        web::http::http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<web::http::http_response> makePutRequest(
        const std::string& path,
        const web::json::value& body,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
        web::http::http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<web::http::http_response> makeDeleteRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
    std::shared_ptr<MockStateService> mockStateService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(StatesCrudTestSuite, StatesTestFixture)

// ============================================================
// GET /api/states — Получение списка состояний
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_states_returns_list)
{
    services::StatesPage testPage;
    dto::State s1;
    s1.id = 1;
    s1.workflowId = 10;
    s1.caption = "To Do";
    s1.orderNumber = 1;
    s1.weight = 0;

    dto::State s2;
    s2.id = 2;
    s2.workflowId = 10;
    s2.caption = "Done";
    s2.orderNumber = 2;
    s2.weight = 100;

    testPage.states = { s1, s2 };
    testPage.totalCount = 2;
    mockStateService->setStatesResult(testPage);

    auto response = makeGetRequest("/api/states").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStateService->getStatesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_states_with_workflow_filter)
{
    services::StatesPage emptyPage;
    mockStateService->setStatesResult(emptyPage);

    auto response = makeGetRequest("/api/states?workflowId=10").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockStateService->getLastStatesWorkflowId().has_value());
    BOOST_CHECK_EQUAL(*mockStateService->getLastStatesWorkflowId(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_states_with_pagination)
{
    services::StatesPage emptyPage;
    mockStateService->setStatesResult(emptyPage);

    auto response = makeGetRequest("/api/states?page=3&pageSize=10").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStateService->getLastStatesPage(), 3);
    BOOST_CHECK_EQUAL(mockStateService->getLastStatesPageSize(), 10);
}

// ============================================================
// GET /api/states/{id} — Получение состояния по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_state_by_id_success)
{
    dto::State state;
    state.id = 5;
    state.workflowId = 10;
    state.caption = "In Progress";
    state.weight = 50;
    mockStateService->setStateResult(state);

    auto response = makeGetRequest("/api/states/5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStateService->getLastStateId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("In Progress"));
    BOOST_CHECK_EQUAL(json.at(U("weight")).as_integer(), 50);
}

BOOST_AUTO_TEST_CASE(test_get_state_not_found)
{
    mockStateService->setStateResult(std::nullopt);

    auto response = makeGetRequest("/api/states/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/states — Создание состояния
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_state_success)
{
    dto::State createdState;
    createdState.id = 200;
    createdState.workflowId = 10;
    createdState.caption = "Review";
    createdState.orderNumber = 3;
    createdState.weight = 75;
    mockStateService->setCreateStateResult(createdState);

    web::json::value body;
    body[U("workflowId")] = web::json::value::number(10);
    body[U("caption")] = web::json::value::string(U("Review"));
    body[U("orderNumber")] = web::json::value::number(3);
    body[U("weight")] = web::json::value::number(75);

    auto response = makePostRequest("/api/states", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockStateService->getCreateStateCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockStateService->getLastCreatedState().caption, "Review");
    BOOST_CHECK_EQUAL(*mockStateService->getLastCreatedState().workflowId, 10);
    BOOST_CHECK_EQUAL(mockStateService->getLastCreateStateUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_state_missing_required_fields)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("No WorkflowId"));

    auto response = makePostRequest("/api/states", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockStateService->getCreateStateCallCount(), 0);
}

// ============================================================
// PUT /api/states/{id} — Обновление состояния
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_state_success)
{
    dto::State updatedState;
    updatedState.id = 5;
    updatedState.weight = 60;
    updatedState.isQueue = true;
    mockStateService->setUpdateStateResult(updatedState);

    web::json::value body;
    body[U("weight")] = web::json::value::number(60);
    body[U("isQueue")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/states/5", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStateService->getUpdateStateCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockStateService->getLastUpdatedState().id, 5);
    BOOST_CHECK_EQUAL(mockStateService->getLastUpdateStateUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_state_not_found)
{
    mockStateService->setUpdateStateResult(std::nullopt);

    web::json::value body;
    body[U("weight")] = web::json::value::number(50);

    auto response = makePutRequest("/api/states/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/states/{id} — Удаление состояния
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_state_success)
{
    services::StateResult deleteResult;
    deleteResult.success = true;
    mockStateService->setDeleteStateResult(deleteResult);

    auto response = makeDeleteRequest("/api/states/5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockStateService->getDeleteStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockStateService->getLastDeletedStateId(), 5);
    BOOST_CHECK_EQUAL(mockStateService->getLastDeleteStateUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_state_with_dependencies)
{
    services::StateResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 409;
    deleteResult.errorMessage = "Cannot delete: state is used by items";
    mockStateService->setDeleteStateResult(deleteResult);

    auto response = makeDeleteRequest("/api/states/5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
}

BOOST_AUTO_TEST_CASE(test_delete_state_requires_auth)
{
    auto response = makeDeleteRequest("/api/states/5", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockStateService->getDeleteStateCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_state_lifecycle)
{
    // 1. Создание
    dto::State newState;
    newState.id = 300;
    newState.workflowId = 10;
    newState.caption = "Testing";
    newState.weight = 50;
    mockStateService->setCreateStateResult(newState);

    web::json::value createBody;
    createBody[U("workflowId")] = web::json::value::number(10);
    createBody[U("caption")] = web::json::value::string(U("Testing"));
    createBody[U("weight")] = web::json::value::number(50);

    auto createResponse = makePostRequest("/api/states", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockStateService->setStateResult(newState);
    auto getResponse = makeGetRequest("/api/states/300").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::State updatedState = newState;
    updatedState.weight = 75;
    mockStateService->setUpdateStateResult(updatedState);

    web::json::value updateBody;
    updateBody[U("weight")] = web::json::value::number(75);

    auto updateResponse = makePutRequest("/api/states/300", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    services::StateResult deleteResult;
    deleteResult.success = true;
    mockStateService->setDeleteStateResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/states/300").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockStateService->setStateResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/states/300").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
