// файл: tests/rest_server/test_rest_server.cpp

#define BOOST_TEST_MODULE RestServerTests
#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cpprest/http_client.h>
#include <thread>

#include "api/rest_server.h"
#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server
{
namespace tests
{

struct RestServerFixture
{
    RestServerFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();

        // Настройка по умолчанию
        mockAuthMiddleware->setValidateRequestResult(true, "test_user_123");

        server = std::make_unique<RestServer>("127.0.0.1", 18080);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);

        BOOST_REQUIRE(server->initialize());

        // Запускаем сервер в отдельном потоке
        serverThread = std::thread([this]()
                                   { server->start(); });

        // Даем серверу время на запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~RestServerFixture()
    {
        if (server)
        {
            server->stop();
        }
        if (serverThread.joinable())
        {
            serverThread.join();
        }
    }

    http_response makeRequest(const std::string& path, const method& method = methods::GET, const std::string& token = "")
    {
        http_client client(U("http://127.0.0.1:18080"));
        http_request request(method);
        request.set_request_uri(U(path));

        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }

        return client.request(request).get();
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RestServerTestSuite, RestServerFixture)

BOOST_AUTO_TEST_CASE(test_health_endpoint_returns_ok)
{
    auto response = makeRequest("/health");
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("status")).as_string(), U("ok"));
}

BOOST_AUTO_TEST_CASE(test_login_with_valid_credentials)
{
    services::AuthResult authResult;
    authResult.success = true;
    authResult.accessToken = "test_token_123";

    mockAuthService->setLoginResult(authResult);

    http_client client(U("http://127.0.0.1:18080"));
    http_request request(methods::POST);
    request.set_request_uri(U("/auth/login"));

    web::json::value body;
    body[U("login")] = web::json::value::string(U("testuser"));
    body[U("password")] = web::json::value::string(U("testpass"));
    request.set_body(body);

    auto response = client.request(request).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    BOOST_CHECK_EQUAL(mockAuthService->getLoginCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthService->getLastLogin(), "testuser");
    BOOST_CHECK_EQUAL(mockAuthService->getLastPassword(), "testpass");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("access_token")).as_string(), U("test_token_123"));
}

BOOST_AUTO_TEST_CASE(test_login_with_invalid_credentials)
{
    services::AuthResult authResult;
    authResult.success = false;
    authResult.errorMessage = "Invalid credentials";
    authResult.errorCode = 401;

    mockAuthService->setLoginResult(authResult);

    http_client client(U("http://127.0.0.1:18080"));
    http_request request(methods::POST);
    request.set_request_uri(U("/auth/login"));

    web::json::value body;
    body[U("login")] = web::json::value::string(U("baduser"));
    body[U("password")] = web::json::value::string(U("badpass"));
    request.set_body(body);

    auto response = client.request(request).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

BOOST_AUTO_TEST_CASE(test_protected_endpoint_requires_auth)
{
    // Без токена - middleware не должен вызываться
    auto response = makeRequest("/api/items");
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

BOOST_AUTO_TEST_CASE(test_protected_endpoint_with_valid_token)
{
    auto response = makeRequest("/api/items", methods::GET, "valid_token");
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockAuthMiddleware->getValidateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthMiddleware->getLastAuthHeader(), "Bearer valid_token");
}

BOOST_AUTO_TEST_CASE(test_logout_endpoint)
{
    mockAuthService->setLogoutResult(true);

    http_client client(U("http://127.0.0.1:18080"));
    http_request request(methods::POST);
    request.set_request_uri(U("/auth/logout"));
    request.headers().add(U("Authorization"), U("Bearer token_to_logout"));

    auto response = client.request(request).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockAuthService->getLogoutCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthService->getLastToken(), "token_to_logout");
}

BOOST_AUTO_TEST_CASE(test_change_password_endpoint)
{
    services::ChangePasswordResult result;
    result.success = true;
    mockAuthService->setChangePasswordResult(result);

    // Настраиваем middleware для получения userId
    mockAuthMiddleware->setValidateRequestResult(true, "123");

    http_client client(U("http://127.0.0.1:18080"));
    http_request request(methods::POST);
    request.set_request_uri(U("/auth/change-password"));
    request.headers().add(U("Authorization"), U("Bearer valid_token"));

    web::json::value body;
    body[U("old_password")] = web::json::value::string(U("oldpass"));
    body[U("new_password")] = web::json::value::string(U("newpass"));
    request.set_body(body);

    auto response = client.request(request).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockAuthService->getChangePasswordCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthService->getLastUserId(), 123);
}

BOOST_AUTO_TEST_CASE(test_not_found_route_returns_404)
{
    auto response = makeRequest("/non_existent_route");
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
