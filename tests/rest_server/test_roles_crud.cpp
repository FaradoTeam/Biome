#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_role_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RolesTestFixture
{
    RolesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRoleService = std::make_shared<MockRoleService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18091);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRoleService(mockRoleService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~RolesTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    // аналогичные методы makeGetRequest, makePostRequest, makePutRequest, makeDeleteRequest
    // (как в TeamsTestFixture, только порт 18091)
    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18091"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18091"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18091"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18091"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRoleService> mockRoleService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RolesCrudTestSuite, RolesTestFixture)

BOOST_AUTO_TEST_CASE(test_get_roles_returns_list)
{
    services::RolesPage page;
    dto::Role r1;
    r1.id = 1;
    r1.caption = "Admin";
    dto::Role r2;
    r2.id = 2;
    r2.caption = "User";
    page.roles = { r1, r2 };
    page.totalCount = 2;
    mockRoleService->setGetRolesResult(page);

    auto response = makeGetRequest("/api/v1/roles").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleService->getGetRolesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_roles_with_pagination)
{
    services::RolesPage empty;
    mockRoleService->setGetRolesResult(empty);

    auto response = makeGetRequest("/api/v1/roles?page=2&pageSize=5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleService->getLastGetRolesPage(), 2);
    BOOST_CHECK_EQUAL(mockRoleService->getLastGetRolesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_role_by_id_success)
{
    dto::Role role;
    role.id = 7;
    role.caption = "Role7";
    mockRoleService->setGetRoleResult(role);

    auto response = makeGetRequest("/api/v1/roles/7").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleService->getLastGetRoleId(), 7);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 7);
}

BOOST_AUTO_TEST_CASE(test_get_role_not_found)
{
    mockRoleService->setGetRoleResult(std::nullopt);
    auto response = makeGetRequest("/api/v1/roles/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_create_role_success)
{
    dto::Role created;
    created.id = 10;
    created.caption = "NewRole";
    mockRoleService->setCreateRoleResult(created);

    json::value body;
    body[U("caption")] = json::value::string(U("NewRole"));

    auto response = makePostRequest("/api/v1/roles", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(*mockRoleService->getLastCreatedRole().caption, "NewRole");
}

BOOST_AUTO_TEST_CASE(test_create_role_missing_caption)
{
    // Устанавливаем, что сервис не должен быть вызван
    mockRoleService->setCreateRoleResult(std::nullopt);

    json::value body;
    body[U("description")] = json::value::string(U("Some description"));

    auto response = makePostRequest("/api/v1/roles", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRoleService->getCreateRoleCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_update_role_success)
{
    dto::Role updated;
    updated.id = 1;
    updated.caption = "Updated";
    mockRoleService->setUpdateRoleResult(updated);

    json::value body;
    body[U("caption")] = json::value::string(U("Updated"));

    auto response = makePutRequest("/api/v1/roles/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(*mockRoleService->getLastUpdatedRole().id, 1);
}

BOOST_AUTO_TEST_CASE(test_delete_role_success)
{
    mockRoleService->setDeleteRoleResult(true);
    auto response = makeDeleteRequest("/api/v1/roles/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRoleService->getLastDeletedRoleId(), 3);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
