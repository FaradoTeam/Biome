#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_phase_service.h"
#include "tests/server_mocks/mock_project_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct UsersTestFixture
{
    UsersTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockPhaseService = std::make_shared<MockPhaseService>();
        mockProjectService = std::make_shared<MockProjectService>();
        mockUserService = std::make_shared<MockUserService>();

        // Настройка аутентификации по умолчанию (обычный пользователь)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        setupDefaultUserService();

        server = std::make_unique<RestServer>("127.0.0.1", 18081);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setPhaseService(mockPhaseService);
        server->setProjectService(mockProjectService);
        server->setUserService(mockUserService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void setupDefaultUserService()
    {
        services::UsersPage testPage;

        dto::User user1;
        user1.id = 1;
        user1.login = "admin";
        user1.firstName = "Admin";
        user1.lastName = "User";
        user1.email = "admin@example.com";
        user1.isSuperAdmin = true;
        user1.isBlocked = false;
        user1.isHidden = false;

        dto::User user2;
        user2.id = 2;
        user2.login = "testuser";
        user2.firstName = "Test";
        user2.lastName = "User";
        user2.email = "test@example.com";
        user2.isSuperAdmin = false;
        user2.isBlocked = false;
        user2.isHidden = false;

        dto::User user3;
        user3.id = 3;
        user3.login = "regular";
        user3.firstName = "Regular";
        user3.lastName = "User";
        user3.email = "regular@example.com";
        user3.isSuperAdmin = false;
        user3.isBlocked = false;
        user3.isHidden = false;

        testPage.users = { user1, user2, user3 };
        testPage.totalCount = 3;

        mockUserService->setGetUsersResult(testPage);
        mockUserService->setGetUserResult(user1);

        dto::User newUser;
        newUser.id = 100;
        newUser.login = "newuser";
        newUser.firstName = "New";
        newUser.lastName = "User";
        newUser.email = "new@example.com";
        newUser.needChangePassword = true;
        mockUserService->setCreateUserResult(newUser);

        dto::User updatedUser = user1;
        updatedUser.firstName = "UpdatedName";
        mockUserService->setUpdateUserResult(updatedUser);
        mockUserService->setDeleteUserResult(true);
    }

    ~UsersTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18081"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18081"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18081"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18081"));
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
    std::shared_ptr<MockPhaseService> mockPhaseService;
    std::shared_ptr<MockProjectService> mockProjectService;
    std::shared_ptr<MockUserService> mockUserService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UsersCrudTestSuite, UsersTestFixture)

// ============================================================
// GET /api/users — Получение списка пользователей (доступно всем авторизованным)
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_users_returns_list_for_regular_user)
{
    // Обычный авторизованный пользователь может получить список пользователей
    auto response = makeGetRequest("/api/users").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserService->getGetUsersCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUsersUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_users_with_pagination_params)
{
    auto response = makeGetRequest("/api/users?page=2&pageSize=2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUsersPage(), 2);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUsersPageSize(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_users_requires_auth)
{
    auto response = makeGetRequest("/api/users", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserService->getGetUsersCallCount(), 0);
}

// ============================================================
// GET /api/users/{id} — Получение пользователя по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_by_id_success)
{
    dto::User user;
    user.id = 42;
    user.login = "specific_user";
    user.email = "specific@test.com";
    mockUserService->setGetUserResult(user);

    auto response = makeGetRequest("/api/users/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserService->getGetUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUserId(), 42);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUserRequestUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("login")).as_string(), U("specific_user"));
}

BOOST_AUTO_TEST_CASE(test_get_user_not_found)
{
    mockUserService->setGetUserResult(std::nullopt);

    auto response = makeGetRequest("/api/users/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->getGetUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUserId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_user_invalid_id)
{
    auto response = makeGetRequest("/api/users/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/users — Создание пользователя (только для супер-админа)
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_user_fails_for_regular_user)
{
    web::json::value body;
    body[U("login")] = web::json::value::string(U("newuser"));
    body[U("email")] = web::json::value::string(U("new@test.com"));
    body[U("password")] = web::json::value::string(U("securepass123"));
    body[U("firstName")] = web::json::value::string(U("New"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockUserService->getCreateUserCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_success_for_admin)
{
    // Переключаемся на супер-админа
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    web::json::value body;
    body[U("login")] = web::json::value::string(U("newuser"));
    body[U("email")] = web::json::value::string(U("new@test.com"));
    body[U("password")] = web::json::value::string(U("securepass123"));
    body[U("firstName")] = web::json::value::string(U("New"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserService->getCreateUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastCreatedUser().login.value_or(""), "newuser");
    BOOST_CHECK_EQUAL(mockUserService->getLastCreatedPassword(), "securepass123");
    BOOST_CHECK_EQUAL(mockUserService->getLastCreateUserUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK(!json.has_field(U("password")));
}

BOOST_AUTO_TEST_CASE(test_create_user_missing_required_fields)
{
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    web::json::value body;
    body[U("login")] = web::json::value::string(U("incomplete"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserService->getCreateUserCallCount(), 0);
}

// ============================================================
// PUT /api/users/{id} — Обновление пользователя (только для супер-админа)
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_user_fails_for_regular_user)
{
    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("UpdatedName"));

    auto response = makePutRequest("/api/users/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockUserService->getUpdateUserCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_update_user_success_for_admin)
{
    // Переключаемся на супер-админа
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("UpdatedName"));

    auto response = makePutRequest("/api/users/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserService->getUpdateUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastUpdatedUser().id.value_or(0), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastUpdateUserUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_user_not_found_for_admin)
{
    mockAuthMiddleware->setValidateRequestResult(true, "1");
    mockUserService->setUpdateUserResult(std::nullopt);

    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("Ghost"));

    auto response = makePutRequest("/api/users/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->getUpdateUserCallCount(), 1);
}

// ============================================================
// DELETE /api/users/{id} — Удаление пользователя (только для супер-админа)
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_user_fails_for_regular_user)
{
    auto response = makeDeleteRequest("/api/users/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockUserService->getDeleteUserCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_delete_user_success_for_admin)
{
    // Переключаемся на супер-админа
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    auto response = makeDeleteRequest("/api/users/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserService->getDeleteUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastDeletedUserId(), 2);
    BOOST_CHECK_EQUAL(mockUserService->getLastDeleteUserUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_user_not_found_for_admin)
{
    mockAuthMiddleware->setValidateRequestResult(true, "1");
    mockUserService->setDeleteUserResult(false);

    auto response = makeDeleteRequest("/api/users/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->getDeleteUserCallCount(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
