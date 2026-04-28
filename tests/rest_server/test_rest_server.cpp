#define BOOST_TEST_MODULE RestServerTests

#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

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
        mockUserService = std::make_shared<MockUserService>();

        // Настройка по умолчанию
        mockAuthMiddleware->setValidateRequestResult(true, "test_user_123");

        // Настройка UserService с тестовыми данными
        setupDefaultUserService();

        server = std::make_unique<RestServer>("127.0.0.1", 18080);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);

        BOOST_REQUIRE(server->initialize());

        // Запускаем сервер в отдельном потоке
        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );

        // Даем серверу время на запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void setupDefaultUserService()
    {
        // Настройка тестовых пользователей для эндпоинтов /api/users
        services::UsersPage testPage;

        dto::User user1;
        user1.id = 1;
        user1.login = "testuser1";
        user1.firstName = "Test";
        user1.lastName = "User1";
        user1.email = "test1@example.com";
        user1.needChangePassword = false;
        user1.isBlocked = false;
        user1.isSuperAdmin = false;
        user1.isHidden = false;

        dto::User user2;
        user2.id = 2;
        user2.login = "testuser2";
        user2.firstName = "Test";
        user2.lastName = "User2";
        user2.email = "test2@example.com";
        user2.needChangePassword = true;
        user2.isBlocked = false;
        user2.isSuperAdmin = true;
        user2.isHidden = false;

        testPage.users = { user1, user2 };
        testPage.totalCount = 2;

        mockUserService->setGetUsersResult(testPage);
        mockUserService->setGetUserResult(user1);

        dto::User newUser;
        newUser.id = 3;
        newUser.login = "newuser";
        newUser.firstName = "New";
        newUser.lastName = "User";
        newUser.email = "new@example.com";
        newUser.needChangePassword = true;
        mockUserService->setCreateUserResult(newUser);

        mockUserService->setUpdateUserResult(user1);
        mockUserService->setDeleteUserResult(true);
    }

    ~RestServerFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    // Используем полный квалифицированный тип из cpprestsdk
    pplx::task<web::http::http_response> makeRequest(
        const std::string& path,
        const web::http::method& method = methods::GET,
        const std::string& token = "",
        const web::json::value& body = web::json::value::null()
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18080"));
        web::http::http_request request(method);
        request.set_request_uri(U(path));

        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }

        if (!body.is_null())
        {
            request.set_body(body);
        }

        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockUserService> mockUserService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RestServerTestSuite, RestServerFixture)

BOOST_AUTO_TEST_CASE(test_health_endpoint_returns_ok)
{
    auto response = makeRequest("/health").get();
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

    web::http::client::http_client client(U("http://127.0.0.1:18080"));
    web::http::http_request request(methods::POST);
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

    web::http::client::http_client client(U("http://127.0.0.1:18080"));
    web::http::http_request request(methods::POST);
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
    auto response = makeRequest("/api/items").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

BOOST_AUTO_TEST_CASE(test_protected_endpoint_with_valid_token)
{
    auto response = makeRequest("/api/items", methods::GET, "valid_token").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockAuthMiddleware->getValidateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthMiddleware->getLastAuthHeader(), "Bearer valid_token");
}

BOOST_AUTO_TEST_CASE(test_logout_endpoint)
{
    mockAuthService->setLogoutResult(true);

    web::http::client::http_client client(U("http://127.0.0.1:18080"));
    web::http::http_request request(methods::POST);
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

    web::http::client::http_client client(U("http://127.0.0.1:18080"));
    web::http::http_request request(methods::POST);
    request.set_request_uri(U("/auth/change-password"));
    request.headers().add(U("Authorization"), U("Bearer valid_token"));

    web::json::value body;
    body[U("oldPassword")] = web::json::value::string(U("oldpass"));
    body[U("newPassword")] = web::json::value::string(U("newpass"));
    request.set_body(body);

    auto response = client.request(request).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockAuthService->getChangePasswordCallCount(), 1);
    BOOST_CHECK_EQUAL(mockAuthService->getLastUserId(), 123);
}

BOOST_AUTO_TEST_CASE(test_not_found_route_returns_404)
{
    auto response = makeRequest("/non_existent_route").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_users_returns_list)
{
    auto response = makeRequest("/api/users", methods::GET, "valid_token").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_user_by_id_success)
{
    auto response = makeRequest("/api/users/1", methods::GET, "valid_token").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("login")).as_string(), U("testuser1"));
}

BOOST_AUTO_TEST_CASE(test_create_user_success)
{
    web::json::value body;
    body[U("login")] = web::json::value::string(U("newuser"));
    body[U("email")] = web::json::value::string(U("new@test.com"));
    body[U("password")] = web::json::value::string(U("securepass123"));
    body[U("firstName")] = web::json::value::string(U("New"));

    auto response = makeRequest("/api/users", methods::POST, "valid_token", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 3);
    BOOST_CHECK(!json.has_field(U("password")));
}

BOOST_AUTO_TEST_CASE(test_update_user_success)
{
    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("UpdatedName"));

    auto response = makeRequest("/api/users/1", methods::PUT, "valid_token", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_CASE(test_delete_user_success)
{
    auto response = makeRequest("/api/users/2", methods::DEL, "valid_token").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
