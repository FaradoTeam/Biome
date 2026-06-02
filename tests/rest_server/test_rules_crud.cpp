#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_rule_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RulesTestFixture
{
    RulesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRuleService = std::make_shared<MockRuleService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18092);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRuleService(mockRuleService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~RulesTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18092"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18092"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18092"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18092"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRuleService> mockRuleService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RulesCrudTestSuite, RulesTestFixture)

// ============================================================
// GET /api/rules — Получение списка правил
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rules_returns_list)
{
    services::RulesPage testPage;
    dto::Rule r1;
    r1.id = 1;
    r1.roleId = 10;
    r1.isRootProjectCreator = true;
    dto::Rule r2;
    r2.id = 2;
    r2.roleId = 20;
    r2.isRootProjectCreator = false;
    testPage.rules = { r1, r2 };
    testPage.totalCount = 2;
    mockRuleService->setGetRulesResult(testPage);

    auto response = makeGetRequest("/api/rules").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleService->getGetRulesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_rules_with_pagination)
{
    services::RulesPage emptyPage;
    mockRuleService->setGetRulesResult(emptyPage);

    auto response = makeGetRequest("/api/rules?page=3&pageSize=10").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleService->getLastGetRulesPage(), 3);
    BOOST_CHECK_EQUAL(mockRuleService->getLastGetRulesPageSize(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_rules_with_role_filter)
{
    services::RulesPage emptyPage;
    mockRuleService->setGetRulesResult(emptyPage);

    auto response = makeGetRequest("/api/rules?roleId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleService->getLastGetRulesRoleId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleService->getLastGetRulesRoleId(), 42);
}

// ============================================================
// GET /api/rules/{id} — Получение правила по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_by_id_success)
{
    dto::Rule rule;
    rule.id = 5;
    rule.roleId = 10;
    rule.isRootProjectCreator = true;
    mockRuleService->setGetRuleResult(rule);

    auto response = makeGetRequest("/api/rules/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleService->getLastGetRuleId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("roleId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("isRootProjectCreator")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_rule_not_found)
{
    mockRuleService->setGetRuleResult(std::nullopt);

    auto response = makeGetRequest("/api/rules/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/rules — Создание правила
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_rule_success)
{
    dto::Rule created;
    created.id = 10;
    created.roleId = 5;
    mockRuleService->setCreateRuleResult(created);

    json::value body;
    body[U("roleId")] = json::value::number(5);
    body[U("isRootProjectCreator")] = json::value::boolean(true);

    auto response = makePostRequest("/api/rules", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockRuleService->getCreateRuleCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleService->getLastCreatedRule().roleId, 5);
}

BOOST_AUTO_TEST_CASE(test_create_rule_missing_role_id)
{
    mockRuleService->setCreateRuleResult(std::nullopt);

    json::value body;
    body[U("isRootProjectCreator")] = json::value::boolean(true);

    auto response = makePostRequest("/api/rules", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRuleService->getCreateRuleCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_rule_duplicate)
{
    mockRuleService->setCreateRuleResult(std::nullopt);

    json::value body;
    body[U("roleId")] = json::value::number(5);

    auto response = makePostRequest("/api/rules", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden); // TODO: status_codes::Conflict
    BOOST_CHECK_EQUAL(mockRuleService->getCreateRuleCallCount(), 1);
}

// ============================================================
// PUT /api/rules/{id} — Обновление правила
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_rule_success)
{
    dto::Rule updated;
    updated.id = 1;
    updated.isRootProjectCreator = true;
    mockRuleService->setUpdateRuleResult(updated);

    json::value body;
    body[U("isRootProjectCreator")] = json::value::boolean(true);

    auto response = makePutRequest("/api/rules/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleService->getUpdateRuleCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleService->getLastUpdatedRule().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_rule_not_found)
{
    mockRuleService->setUpdateRuleResult(std::nullopt);

    json::value body;
    body[U("isRootProjectCreator")] = json::value::boolean(true);

    auto response = makePutRequest("/api/rules/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/rules/{id} — Удаление правила
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_rule_success)
{
    mockRuleService->setDeleteRuleResult(true);

    auto response = makeDeleteRequest("/api/rules/2").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRuleService->getDeleteRuleCallCount(), 1);
    BOOST_CHECK_EQUAL(mockRuleService->getLastDeletedRuleId(), 2);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_not_found)
{
    mockRuleService->setDeleteRuleResult(false);

    auto response = makeDeleteRequest("/api/rules/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_requires_auth)
{
    auto response = makeDeleteRequest("/api/rules/1", "").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockRuleService->getDeleteRuleCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_rule_lifecycle)
{
    // 1. Создание
    dto::Rule newRule;
    newRule.id = 100;
    newRule.roleId = 50;
    mockRuleService->setCreateRuleResult(newRule);

    json::value createBody;
    createBody[U("roleId")] = json::value::number(50);

    auto createResponse = makePostRequest("/api/rules", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockRuleService->setGetRuleResult(newRule);
    auto getResponse = makeGetRequest("/api/rules/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::Rule updatedRule = newRule;
    updatedRule.isRootProjectCreator = true;
    mockRuleService->setUpdateRuleResult(updatedRule);

    json::value updateBody;
    updateBody[U("isRootProjectCreator")] = json::value::boolean(true);

    auto updateResponse = makePutRequest("/api/rules/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    mockRuleService->setDeleteRuleResult(true);
    auto deleteResponse = makeDeleteRequest("/api/rules/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockRuleService->setGetRuleResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/rules/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
