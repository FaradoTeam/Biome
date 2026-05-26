#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_team_role_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct UserTeamRolesTestFixture
{
    UserTeamRolesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserTeamRoleService = std::make_shared<MockUserTeamRoleService>();

        mockAuthMiddleware->setValidateRequestResult(true, "test_user_123");

        server = std::make_unique<RestServer>("127.0.0.1", 18097);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserTeamRoleService(mockUserTeamRoleService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread([this]()
                                   { server->start(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~UserTeamRolesTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18097"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18097"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18097"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18097"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockUserTeamRoleService> mockUserTeamRoleService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UserTeamRolesCrudTestSuite, UserTeamRolesTestFixture)

// ============================================================
// GET /api/user-team-roles — Получение списка назначений
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_team_roles_returns_list)
{
    services::UserTeamRolesPage testPage;
    dto::UserTeamRole utr1;
    utr1.id = 1;
    utr1.userId = 100;
    utr1.teamId = 10;
    utr1.roleId = 1;
    dto::UserTeamRole utr2;
    utr2.id = 2;
    utr2.userId = 200;
    utr2.teamId = 20;
    utr2.roleId = 2;
    testPage.items = { utr1, utr2 };
    testPage.totalCount = 2;
    mockUserTeamRoleService->setGetUserTeamRolesResult(testPage);

    auto response = makeGetRequest("/api/user-team-roles").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getGetUserTeamRolesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_user_team_roles_with_pagination)
{
    services::UserTeamRolesPage emptyPage;
    mockUserTeamRoleService->setGetUserTeamRolesResult(emptyPage);

    auto response = makeGetRequest("/api/user-team-roles?page=2&pageSize=15").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getLastGetUserTeamRolesPage(), 2);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getLastGetUserTeamRolesPageSize(), 15);
}

BOOST_AUTO_TEST_CASE(test_get_user_team_roles_with_user_filter)
{
    services::UserTeamRolesPage emptyPage;
    mockUserTeamRoleService->setGetUserTeamRolesResult(emptyPage);

    auto response = makeGetRequest("/api/user-team-roles?userId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTeamRoleService->getLastGetUserTeamRolesUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastGetUserTeamRolesUserId(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_user_team_roles_with_team_filter)
{
    services::UserTeamRolesPage emptyPage;
    mockUserTeamRoleService->setGetUserTeamRolesResult(emptyPage);

    auto response = makeGetRequest("/api/user-team-roles?teamId=10").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTeamRoleService->getLastGetUserTeamRolesTeamId().has_value());
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastGetUserTeamRolesTeamId(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_user_team_roles_with_role_filter)
{
    services::UserTeamRolesPage emptyPage;
    mockUserTeamRoleService->setGetUserTeamRolesResult(emptyPage);

    auto response = makeGetRequest("/api/user-team-roles?roleId=5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTeamRoleService->getLastGetUserTeamRolesRoleId().has_value());
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastGetUserTeamRolesRoleId(), 5);
}

// ============================================================
// GET /api/user-team-roles/{id} — Получение назначения по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_team_role_by_id_success)
{
    dto::UserTeamRole utr;
    utr.id = 5;
    utr.userId = 100;
    utr.teamId = 10;
    utr.roleId = 1;
    mockUserTeamRoleService->setGetUserTeamRoleResult(utr);

    auto response = makeGetRequest("/api/user-team-roles/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getLastGetUserTeamRoleId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("teamId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("roleId")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_user_team_role_not_found)
{
    mockUserTeamRoleService->setGetUserTeamRoleResult(std::nullopt);

    auto response = makeGetRequest("/api/user-team-roles/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/user-team-roles — Создание назначения
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_user_team_role_success)
{
    dto::UserTeamRole created;
    created.id = 10;
    created.userId = 5;
    created.teamId = 50;
    created.roleId = 3;
    mockUserTeamRoleService->setCreateUserTeamRoleResult(created);

    json::value body;
    body[U("userId")] = json::value::number(5);
    body[U("teamId")] = json::value::number(50);
    body[U("roleId")] = json::value::number(3);

    auto response = makePostRequest("/api/user-team-roles", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getCreateUserTeamRoleCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastCreatedUserTeamRole().userId, 5);
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastCreatedUserTeamRole().teamId, 50);
}

BOOST_AUTO_TEST_CASE(test_create_user_team_role_missing_required_fields)
{
    mockUserTeamRoleService->setCreateUserTeamRoleResult(std::nullopt);

    json::value body;
    body[U("userId")] = json::value::number(5);
    body[U("teamId")] = json::value::number(50);

    auto response = makePostRequest("/api/user-team-roles", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getCreateUserTeamRoleCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_team_role_duplicate)
{
    mockUserTeamRoleService->setCreateUserTeamRoleResult(std::nullopt);

    json::value body;
    body[U("userId")] = json::value::number(5);
    body[U("teamId")] = json::value::number(50);
    body[U("roleId")] = json::value::number(3);

    auto response = makePostRequest("/api/user-team-roles", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
}

// ============================================================
// PUT /api/user-team-roles/{id} — Обновление назначения
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_user_team_role_success)
{
    dto::UserTeamRole updated;
    updated.id = 1;
    updated.roleId = 5;
    mockUserTeamRoleService->setUpdateUserTeamRoleResult(updated);

    json::value body;
    body[U("roleId")] = json::value::number(5);

    auto response = makePutRequest("/api/user-team-roles/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getUpdateUserTeamRoleCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockUserTeamRoleService->getLastUpdatedUserTeamRole().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_user_team_role_not_found)
{
    mockUserTeamRoleService->setUpdateUserTeamRoleResult(std::nullopt);

    json::value body;
    body[U("roleId")] = json::value::number(5);

    auto response = makePutRequest("/api/user-team-roles/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/user-team-roles/{id} — Удаление назначения
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_user_team_role_success)
{
    mockUserTeamRoleService->setDeleteUserTeamRoleResult(true);

    auto response = makeDeleteRequest("/api/user-team-roles/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getDeleteUserTeamRoleCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getLastDeletedUserTeamRoleId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_user_team_role_not_found)
{
    mockUserTeamRoleService->setDeleteUserTeamRoleResult(false);

    auto response = makeDeleteRequest("/api/user-team-roles/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_user_team_role_requires_auth)
{
    auto response = makeDeleteRequest("/api/user-team-roles/1", "").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserTeamRoleService->getDeleteUserTeamRoleCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_user_team_role_lifecycle)
{
    // 1. Создание
    dto::UserTeamRole newUtr;
    newUtr.id = 100;
    newUtr.userId = 50;
    newUtr.teamId = 500;
    newUtr.roleId = 10;
    mockUserTeamRoleService->setCreateUserTeamRoleResult(newUtr);

    json::value createBody;
    createBody[U("userId")] = json::value::number(50);
    createBody[U("teamId")] = json::value::number(500);
    createBody[U("roleId")] = json::value::number(10);

    auto createResponse = makePostRequest("/api/user-team-roles", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockUserTeamRoleService->setGetUserTeamRoleResult(newUtr);
    auto getResponse = makeGetRequest("/api/user-team-roles/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::UserTeamRole updatedUtr = newUtr;
    updatedUtr.roleId = 20;
    mockUserTeamRoleService->setUpdateUserTeamRoleResult(updatedUtr);

    json::value updateBody;
    updateBody[U("roleId")] = json::value::number(20);

    auto updateResponse = makePutRequest("/api/user-team-roles/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    mockUserTeamRoleService->setDeleteUserTeamRoleResult(true);
    auto deleteResponse = makeDeleteRequest("/api/user-team-roles/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
