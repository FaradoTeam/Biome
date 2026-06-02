#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_rule_project_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RuleProjectsTestFixture
{
    RuleProjectsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRuleProjectService = std::make_shared<MockRuleProjectService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18093);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRuleProjectService(mockRuleProjectService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~RuleProjectsTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18093"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18093"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18093"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18093"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRuleProjectService> mockRuleProjectService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RuleProjectsCrudTestSuite, RuleProjectsTestFixture)

// ============================================================
// GET /api/rule-projects — Получение списка прав на проекты
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_projects_returns_list)
{
    services::RuleProjectsPage testPage;
    dto::RuleProject rp1;
    rp1.id = 1;
    rp1.ruleId = 10;
    rp1.projectId = 100;
    rp1.isReader = true;
    dto::RuleProject rp2;
    rp2.id = 2;
    rp2.ruleId = 20;
    rp2.projectId = 200;
    rp2.isWriter = true;
    testPage.items = { rp1, rp2 };
    testPage.totalCount = 2;
    mockRuleProjectService->setGetRuleProjectsResult(testPage);

    auto response = makeGetRequest("/api/rule-projects").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getGetRuleProjectsCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_rule_projects_with_pagination)
{
    services::RuleProjectsPage emptyPage;
    mockRuleProjectService->setGetRuleProjectsResult(emptyPage);

    auto response = makeGetRequest("/api/rule-projects?page=2&pageSize=15").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getLastGetRuleProjectsPage(), 2);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getLastGetRuleProjectsPageSize(), 15);
}

BOOST_AUTO_TEST_CASE(test_get_rule_projects_with_rule_filter)
{
    services::RuleProjectsPage emptyPage;
    mockRuleProjectService->setGetRuleProjectsResult(emptyPage);

    auto response = makeGetRequest("/api/rule-projects?ruleId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleProjectService->getLastGetRuleProjectsRuleId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleProjectService->getLastGetRuleProjectsRuleId(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_rule_projects_with_project_filter)
{
    services::RuleProjectsPage emptyPage;
    mockRuleProjectService->setGetRuleProjectsResult(emptyPage);

    auto response = makeGetRequest("/api/rule-projects?projectId=100").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleProjectService->getLastGetRuleProjectsProjectId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleProjectService->getLastGetRuleProjectsProjectId(), 100);
}

// ============================================================
// GET /api/rule-projects/{id} — Получение права по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_project_by_id_success)
{
    dto::RuleProject rp;
    rp.id = 5;
    rp.ruleId = 10;
    rp.projectId = 100;
    rp.isWriter = true;
    mockRuleProjectService->setGetRuleProjectResult(rp);

    auto response = makeGetRequest("/api/rule-projects/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getLastGetRuleProjectId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("ruleId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_rule_project_not_found)
{
    mockRuleProjectService->setGetRuleProjectResult(std::nullopt);

    auto response = makeGetRequest("/api/rule-projects/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/rule-projects — Создание права на проект
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_rule_project_success)
{
    dto::RuleProject created;
    created.id = 10;
    created.ruleId = 5;
    created.projectId = 50;
    mockRuleProjectService->setCreateRuleProjectResult(created);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("projectId")] = json::value::number(50);
    body[U("isReader")] = json::value::boolean(true);
    body[U("isWriter")] = json::value::boolean(true);

    auto response = makePostRequest("/api/rule-projects", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getCreateRuleProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleProjectService->getLastCreatedRuleProject().ruleId, 5);
    BOOST_CHECK_EQUAL(*mockRuleProjectService->getLastCreatedRuleProject().projectId, 50);
}

BOOST_AUTO_TEST_CASE(test_create_rule_project_missing_required_fields)
{
    mockRuleProjectService->setCreateRuleProjectResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);

    auto response = makePostRequest("/api/rule-projects", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRuleProjectService->getCreateRuleProjectCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_rule_project_duplicate)
{
    mockRuleProjectService->setCreateRuleProjectResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("projectId")] = json::value::number(50);

    auto response = makePostRequest("/api/rule-projects", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden); // TODO: status_codes::Conflict
}

// ============================================================
// PUT /api/rule-projects/{id} — Обновление права на проект
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_rule_project_success)
{
    dto::RuleProject updated;
    updated.id = 1;
    updated.isReader = true;
    updated.isWriter = false;
    mockRuleProjectService->setUpdateRuleProjectResult(updated);

    json::value body;
    body[U("isReader")] = json::value::boolean(true);
    body[U("isWriter")] = json::value::boolean(false);

    auto response = makePutRequest("/api/rule-projects/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getUpdateRuleProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleProjectService->getLastUpdatedRuleProject().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_rule_project_not_found)
{
    mockRuleProjectService->setUpdateRuleProjectResult(std::nullopt);

    json::value body;
    body[U("isReader")] = json::value::boolean(true);

    auto response = makePutRequest("/api/rule-projects/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/rule-projects/{id} — Удаление права на проект
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_rule_project_success)
{
    mockRuleProjectService->setDeleteRuleProjectResult(true);

    auto response = makeDeleteRequest("/api/rule-projects/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getDeleteRuleProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockRuleProjectService->getLastDeletedRuleProjectId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_project_not_found)
{
    mockRuleProjectService->setDeleteRuleProjectResult(false);

    auto response = makeDeleteRequest("/api/rule-projects/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// Интеграционный тест
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_rule_project_lifecycle)
{
    // 1. Создание
    dto::RuleProject newRp;
    newRp.id = 100;
    newRp.ruleId = 50;
    newRp.projectId = 500;
    mockRuleProjectService->setCreateRuleProjectResult(newRp);

    json::value createBody;
    createBody[U("ruleId")] = json::value::number(50);
    createBody[U("projectId")] = json::value::number(500);

    auto createResponse = makePostRequest("/api/rule-projects", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockRuleProjectService->setGetRuleProjectResult(newRp);
    auto getResponse = makeGetRequest("/api/rule-projects/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::RuleProject updatedRp = newRp;
    updatedRp.isReader = true;
    mockRuleProjectService->setUpdateRuleProjectResult(updatedRp);

    json::value updateBody;
    updateBody[U("isReader")] = json::value::boolean(true);

    auto updateResponse = makePutRequest("/api/rule-projects/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    mockRuleProjectService->setDeleteRuleProjectResult(true);
    auto deleteResponse = makeDeleteRequest("/api/rule-projects/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
