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

struct UsersTestFixture
{
    UsersTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();

        mockAuthMiddleware->setValidateRequestResult(true, "test_user_123");

        server = std::make_unique<RestServer>("127.0.0.1", 18081);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
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
    std::shared_ptr<MockUserService> mockUserService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UsersCrudTestSuite, UsersTestFixture)

// ============================================================
// GET /api/users — Получение списка пользователей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_users_returns_list)
{
    services::UsersPage testPage;
    dto::User user1;
    user1.id = 1;
    user1.login = "user1";
    user1.email = "user1@test.com";

    dto::User user2;
    user2.id = 2;
    user2.login = "user2";
    user2.email = "user2@test.com";

    testPage.users = { user1, user2 };
    testPage.totalCount = 2;
    mockUserService->setGetUsersResult(testPage);

    auto response = makeGetRequest("/api/users").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserService->getUsersCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK(json.at(U("items")).is_array());
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_users_with_pagination_params)
{
    services::UsersPage emptyPage;
    mockUserService->setGetUsersResult(emptyPage);

    auto response = makeGetRequest("/api/users?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUsersPage(), 3);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUsersPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_users_empty_list)
{
    services::UsersPage emptyPage;
    mockUserService->setGetUsersResult(emptyPage);

    auto response = makeGetRequest("/api/users").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_users_requires_auth)
{
    auto response = makeGetRequest("/api/users", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserService->getUsersCallCount(), 0);
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
    BOOST_CHECK_EQUAL(mockUserService->getUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUserId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("login")).as_string(), U("specific_user"));
}

BOOST_AUTO_TEST_CASE(test_get_user_not_found)
{
    mockUserService->setGetUserResult(std::nullopt);

    auto response = makeGetRequest("/api/users/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->getUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastGetUserId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_user_invalid_id)
{
    auto response = makeGetRequest("/api/users/invalid").get();
    // Ожидаем 404, потому что маршрут не совпадает с regex \d+
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_user_negative_id)
{
    auto response = makeGetRequest("/api/users/-1").get();
    // Отрицательное число не совпадает с regex \d+, поэтому 404
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/users — Создание пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_user_success)
{
    dto::User createdUser;
    createdUser.id = 100;
    createdUser.login = "newuser";
    createdUser.firstName = "New";
    createdUser.lastName = "User";
    createdUser.email = "new@test.com";
    createdUser.needChangePassword = true;
    mockUserService->setCreateUserResult(createdUser);

    web::json::value body;
    body[U("login")] = web::json::value::string(U("newuser"));
    body[U("email")] = web::json::value::string(U("new@test.com"));
    body[U("password")] = web::json::value::string(U("securepass123"));
    body[U("firstName")] = web::json::value::string(U("New"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserService->createUserCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockUserService->getLastCreatedUser().login, "newuser");
    BOOST_CHECK_EQUAL(mockUserService->getLastCreatedPassword(), "securepass123");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("login")).as_string(), U("newuser"));
    // Пароль не должен возвращаться!
    BOOST_CHECK(!json.has_field(U("password")));
}

BOOST_AUTO_TEST_CASE(test_create_user_missing_required_fields)
{
    web::json::value body;
    body[U("login")] = web::json::value::string(U("incomplete"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserService->createUserCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_duplicate)
{
    mockUserService->setCreateUserResult(std::nullopt);

    web::json::value body;
    body[U("login")] = web::json::value::string(U("existing"));
    body[U("email")] = web::json::value::string(U("existing@test.com"));
    body[U("password")] = web::json::value::string(U("password123"));

    auto response = makePostRequest("/api/users", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
    BOOST_CHECK_EQUAL(mockUserService->createUserCallCount(), 1);
}

// ============================================================
// PUT /api/users/{id} — Обновление пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_user_success)
{
    dto::User updatedUser;
    updatedUser.id = 1;
    updatedUser.firstName = "UpdatedName";
    mockUserService->setUpdateUserResult(updatedUser);

    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("UpdatedName"));
    body[U("isBlocked")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/users/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserService->updateUserCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockUserService->getLastUpdatedUser().id, 1);
    BOOST_CHECK_EQUAL(*mockUserService->getLastUpdatedUser().firstName, "UpdatedName");
    BOOST_CHECK_EQUAL(*mockUserService->getLastUpdatedUser().isBlocked, true);
}

BOOST_AUTO_TEST_CASE(test_update_user_not_found)
{
    mockUserService->setUpdateUserResult(std::nullopt);

    web::json::value body;
    body[U("firstName")] = web::json::value::string(U("Ghost"));

    auto response = makePutRequest("/api/users/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->updateUserCallCount(), 1);
}

// ============================================================
// DELETE /api/users/{id} — Удаление пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_user_success)
{
    mockUserService->setDeleteUserResult(true);

    auto response = makeDeleteRequest("/api/users/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserService->deleteUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastDeletedUserId(), 2);
}

BOOST_AUTO_TEST_CASE(test_delete_user_not_found)
{
    mockUserService->setDeleteUserResult(false);

    auto response = makeDeleteRequest("/api/users/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserService->deleteUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserService->getLastDeletedUserId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_user_requires_auth)
{
    auto response = makeDeleteRequest("/api/users/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserService->deleteUserCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_user_lifecycle)
{
    // 1. Создание пользователя
    dto::User newUser;
    newUser.id = 100;
    newUser.login = "lifecycle";
    newUser.email = "lifecycle@test.com";
    newUser.firstName = "Life";
    newUser.needChangePassword = true;
    mockUserService->setCreateUserResult(newUser);

    web::json::value createBody;
    createBody[U("login")] = web::json::value::string(U("lifecycle"));
    createBody[U("email")] = web::json::value::string(U("lifecycle@test.com"));
    createBody[U("password")] = web::json::value::string(U("TestPass123"));

    auto createResponse = makePostRequest("/api/users", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданного пользователя
    mockUserService->setGetUserResult(newUser);
    auto getResponse = makeGetRequest("/api/users/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление пользователя
    dto::User updatedUser = newUser;
    updatedUser.firstName = "Updated";
    mockUserService->setUpdateUserResult(updatedUser);

    web::json::value updateBody;
    updateBody[U("firstName")] = web::json::value::string(U("Updated"));

    auto updateResponse = makePutRequest("/api/users/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::NoContent);

    // 4. Удаление пользователя
    mockUserService->setDeleteUserResult(true);
    auto deleteResponse = makeDeleteRequest("/api/users/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что пользователь удален
    mockUserService->setGetUserResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/users/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
