#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_rule_state_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RuleStatesTestFixture
{
    RuleStatesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRuleStateService = std::make_shared<MockRuleStateService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18095);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRuleStateService(mockRuleStateService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~RuleStatesTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18095"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18095"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18095"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18095"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRuleStateService> mockRuleStateService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RuleStatesCrudTestSuite, RuleStatesTestFixture)

// ============================================================
// GET /api/v1/rule-states — Получение списка прав на состояния
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_states_returns_list)
{
    services::RuleStatesPage testPage;
    dto::RuleState rs1;
    rs1.id = 1;
    rs1.ruleId = 10;
    rs1.stateId = 100;
    rs1.isStateAllowed = true;
    dto::RuleState rs2;
    rs2.id = 2;
    rs2.ruleId = 20;
    rs2.stateId = 200;
    rs2.isStateAllowed = false;
    testPage.items = { rs1, rs2 };
    testPage.totalCount = 2;
    mockRuleStateService->setGetRuleStatesResult(testPage);

    auto response = makeGetRequest("/api/v1/rule-states").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleStateService->getGetRuleStatesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_rule_states_with_pagination)
{
    services::RuleStatesPage emptyPage;
    mockRuleStateService->setGetRuleStatesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/rule-states?page=2&pageSize=15").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleStateService->getLastGetRuleStatesPage(), 2);
    BOOST_CHECK_EQUAL(mockRuleStateService->getLastGetRuleStatesPageSize(), 15);
}

BOOST_AUTO_TEST_CASE(test_get_rule_states_with_rule_filter)
{
    services::RuleStatesPage emptyPage;
    mockRuleStateService->setGetRuleStatesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/rule-states?ruleId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleStateService->getLastGetRuleStatesRuleId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleStateService->getLastGetRuleStatesRuleId(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_rule_states_with_state_filter)
{
    services::RuleStatesPage emptyPage;
    mockRuleStateService->setGetRuleStatesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/rule-states?stateId=100").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleStateService->getLastGetRuleStatesStateId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleStateService->getLastGetRuleStatesStateId(), 100);
}

// ============================================================
// GET /api/v1/rule-states/{id} — Получение права по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_state_by_id_success)
{
    dto::RuleState rs;
    rs.id = 5;
    rs.ruleId = 10;
    rs.stateId = 100;
    rs.isStateAllowed = true;
    mockRuleStateService->setGetRuleStateResult(rs);

    auto response = makeGetRequest("/api/v1/rule-states/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleStateService->getLastGetRuleStateId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("ruleId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("stateId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("isStateAllowed")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_rule_state_not_found)
{
    mockRuleStateService->setGetRuleStateResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/rule-states/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/rule-states — Создание права на состояние
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_rule_state_success)
{
    dto::RuleState created;
    created.id = 10;
    created.ruleId = 5;
    created.stateId = 50;
    mockRuleStateService->setCreateRuleStateResult(created);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("stateId")] = json::value::number(50);
    body[U("isStateAllowed")] = json::value::boolean(true);

    auto response = makePostRequest("/api/v1/rule-states", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockRuleStateService->getCreateRuleStateCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleStateService->getLastCreatedRuleState().ruleId, 5);
    BOOST_CHECK_EQUAL(*mockRuleStateService->getLastCreatedRuleState().stateId, 50);
}

BOOST_AUTO_TEST_CASE(test_create_rule_state_missing_required_fields)
{
    mockRuleStateService->setCreateRuleStateResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);

    auto response = makePostRequest("/api/v1/rule-states", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRuleStateService->getCreateRuleStateCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_rule_state_duplicate)
{
    mockRuleStateService->setCreateRuleStateResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("stateId")] = json::value::number(50);

    auto response = makePostRequest("/api/v1/rule-states", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden); // TODO: status_codes::Conflict
}

// ============================================================
// PUT /api/v1/rule-states/{id} — Обновление права на состояние
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_rule_state_success)
{
    dto::RuleState updated;
    updated.id = 1;
    updated.isStateAllowed = false;
    mockRuleStateService->setUpdateRuleStateResult(updated);

    json::value body;
    body[U("isStateAllowed")] = json::value::boolean(false);

    auto response = makePutRequest("/api/v1/rule-states/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleStateService->getUpdateRuleStateCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleStateService->getLastUpdatedRuleState().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_rule_state_not_found)
{
    mockRuleStateService->setUpdateRuleStateResult(std::nullopt);

    json::value body;
    body[U("isStateAllowed")] = json::value::boolean(false);

    auto response = makePutRequest("/api/v1/rule-states/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/rule-states/{id} — Удаление права на состояние
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_rule_state_success)
{
    mockRuleStateService->setDeleteRuleStateResult(true);

    auto response = makeDeleteRequest("/api/v1/rule-states/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRuleStateService->getDeleteRuleStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockRuleStateService->getLastDeletedRuleStateId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_state_not_found)
{
    mockRuleStateService->setDeleteRuleStateResult(false);

    auto response = makeDeleteRequest("/api/v1/rule-states/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_state_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/rule-states/1", "").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockRuleStateService->getDeleteRuleStateCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
