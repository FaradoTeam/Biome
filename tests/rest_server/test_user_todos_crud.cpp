#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_service.h"
#include "tests/server_mocks/mock_user_todo_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct UserTodosTestFixture
{
    UserTodosTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockUserTodoService = std::make_shared<MockUserTodoService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultUserTodoService();

        server = std::make_unique<RestServer>("127.0.0.1", 18122);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setUserTodoService(mockUserTodoService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultUserTodoService()
    {
        // Настройка списка задач
        services::UserTodosPage testPage;

        dto::UserTodo todo1 = MockUserTodoService::createTestTodo(1, 100, "Создать проект", false);
        dto::UserTodo todo2 = MockUserTodoService::createTestTodo(2, 100, "Написать документацию", false);
        dto::UserTodo todo3 = MockUserTodoService::createTestTodo(3, 100, "Провести ревью кода", true);

        testPage.todos = { todo1, todo2, todo3 };
        testPage.totalCount = 3;
        mockUserTodoService->setGetTodosResult(testPage);
        mockUserTodoService->setGetTodoResult(todo1);

        dto::UserTodo newTodo = MockUserTodoService::createTestTodo(100, 100, "Новая задача", false);
        mockUserTodoService->setCreateTodoResult(newTodo);

        dto::UserTodo updatedTodo = todo1;
        updatedTodo.caption = "Обновлённая задача";
        updatedTodo.isDone = true;
        mockUserTodoService->setUpdateTodoResult(updatedTodo);

        services::UserTodoResult deleteResult;
        deleteResult.success = true;
        mockUserTodoService->setDeleteTodoResult(deleteResult);
    }

    ~UserTodosTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18122"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18122"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18122"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18122"));
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
    std::shared_ptr<MockUserTodoService> mockUserTodoService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UserTodosCrudTestSuite, UserTodosTestFixture)

// ============================================================
// GET /api/v1/user-todos — Получение списка задач
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_todos_returns_list)
{
    auto response = makeGetRequest("/api/v1/user-todos").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTodoService->getGetTodosCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastGetTodosUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_todos_with_pagination_params)
{
    services::UserTodosPage emptyPage;
    mockUserTodoService->setGetTodosResult(emptyPage);

    auto response = makeGetRequest("/api/v1/user-todos?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastGetTodosPage(), 3);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastGetTodosPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_todos_filter_by_user)
{
    services::UserTodosPage filteredPage;
    dto::UserTodo todo = MockUserTodoService::createTestTodo(42, 50, "Задача другого пользователя");
    filteredPage.todos = { todo };
    filteredPage.totalCount = 1;
    mockUserTodoService->setGetTodosResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-todos?userId=50").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTodoService->getLastGetTodosFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserTodoService->getLastGetTodosFilterUserId(), 50);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 50);
}

BOOST_AUTO_TEST_CASE(test_get_todos_filter_by_done_status)
{
    services::UserTodosPage filteredPage;
    dto::UserTodo doneTodo = MockUserTodoService::createTestTodo(42, 100, "Выполненная задача", true);
    filteredPage.todos = { doneTodo };
    filteredPage.totalCount = 1;
    mockUserTodoService->setGetTodosResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-todos?isDone=true").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTodoService->getLastGetTodosIsDone().has_value());
    BOOST_CHECK_EQUAL(*mockUserTodoService->getLastGetTodosIsDone(), true);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("isDone")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_todos_filter_by_user_and_done)
{
    services::UserTodosPage filteredPage;
    dto::UserTodo todo = MockUserTodoService::createTestTodo(42, 50, "Невыполненная задача другого пользователя", false);
    filteredPage.todos = { todo };
    filteredPage.totalCount = 1;
    mockUserTodoService->setGetTodosResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-todos?userId=50&isDone=false").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserTodoService->getLastGetTodosFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserTodoService->getLastGetTodosFilterUserId(), 50);
    BOOST_REQUIRE(mockUserTodoService->getLastGetTodosIsDone().has_value());
    BOOST_CHECK_EQUAL(*mockUserTodoService->getLastGetTodosIsDone(), false);
}

BOOST_AUTO_TEST_CASE(test_get_todos_requires_auth)
{
    auto response = makeGetRequest("/api/v1/user-todos", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserTodoService->getGetTodosCallCount(), 0);
}

// ============================================================
// GET /api/v1/user-todos/{id} — Получение задачи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_todo_by_id_success)
{
    dto::UserTodo todo = MockUserTodoService::createTestTodo(
        42, 100, "Конкретная задача", false
    );
    mockUserTodoService->setGetTodoResult(todo);

    auto response = makeGetRequest("/api/v1/user-todos/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTodoService->getGetTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastGetTodoId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретная задача"));
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_todo_not_found)
{
    mockUserTodoService->setGetTodoResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/user-todos/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserTodoService->getGetTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastGetTodoId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_todo_invalid_id)
{
    auto response = makeGetRequest("/api/v1/user-todos/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/user-todos — Создание задачи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_todo_success)
{
    dto::UserTodo createdTodo = MockUserTodoService::createTestTodo(
        100, 100, "Новая задача", false
    );
    mockUserTodoService->setCreateTodoResult(createdTodo);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новая задача"));
    body[U("isDone")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-todos", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserTodoService->getCreateTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastCreateTodoUserId(), 100);
    BOOST_CHECK_EQUAL(*mockUserTodoService->getLastCreateTodo().caption, "Новая задача");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новая задача"));
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_create_todo_with_done_status)
{
    dto::UserTodo createdTodo = MockUserTodoService::createTestTodo(
        101, 100, "Выполненная задача", true
    );
    mockUserTodoService->setCreateTodoResult(createdTodo);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Выполненная задача"));
    body[U("isDone")] = web::json::value::boolean(true);

    auto response = makePostRequest("/api/v1/user-todos", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("isDone")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_create_todo_missing_required_fields)
{
    web::json::value body;
    body[U("isDone")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-todos", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserTodoService->getCreateTodoCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_todo_empty_caption)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U(""));

    auto response = makePostRequest("/api/v1/user-todos", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserTodoService->getCreateTodoCallCount(), 0);
}

// ============================================================
// PUT /api/v1/user-todos/{id} — Обновление задачи
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_todo_success)
{
    dto::UserTodo updatedTodo = MockUserTodoService::createTestTodo(
        1, 100, "Обновлённая задача", true
    );
    mockUserTodoService->setUpdateTodoResult(updatedTodo);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновлённая задача"));
    body[U("isDone")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/user-todos/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTodoService->getUpdateTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastUpdateTodoId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновлённая задача"));
    BOOST_CHECK_EQUAL(json.at(U("isDone")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_update_todo_partial)
{
    dto::UserTodo updatedTodo = MockUserTodoService::createTestTodo(
        1, 100, "Заголовок не меняется", true
    );
    mockUserTodoService->setUpdateTodoResult(updatedTodo);

    web::json::value body;
    body[U("isDone")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/user-todos/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserTodoService->getUpdateTodoCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_todo_empty_caption_fails)
{
    mockUserTodoService->setUpdateTodoResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U(""));

    auto response = makePutRequest("/api/v1/user-todos/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserTodoService->getUpdateTodoCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_update_todo_not_found)
{
    // Обновление возвращает nullopt (ошибка)
    mockUserTodoService->setUpdateTodoResult(std::nullopt);
    // И getTodo тоже возвращает nullopt (задача не найдена)
    mockUserTodoService->setGetTodoResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующая задача"));

    auto response = makePutRequest("/api/v1/user-todos/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserTodoService->getUpdateTodoCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/user-todos/{id} — Удаление задачи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_todo_success)
{
    services::UserTodoResult deleteResult;
    deleteResult.success = true;
    mockUserTodoService->setDeleteTodoResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-todos/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserTodoService->getDeleteTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastDeleteTodoId(), 3);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastDeleteTodoUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_todo_not_found)
{
    services::UserTodoResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Todo not found";
    mockUserTodoService->setDeleteTodoResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-todos/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserTodoService->getDeleteTodoCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserTodoService->getLastDeleteTodoId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_todo_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::UserTodoResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockUserTodoService->setDeleteTodoResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-todos/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_todo_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/user-todos/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserTodoService->getDeleteTodoCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_todo_lifecycle)
{
    // 1. Создание задачи
    dto::UserTodo newTodo = MockUserTodoService::createTestTodo(
        200, 100, "Жизненный цикл задачи", false
    );
    mockUserTodoService->setCreateTodoResult(newTodo);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл задачи"));
    createBody[U("isDone")] = web::json::value::boolean(false);

    auto createResponse = makePostRequest("/api/v1/user-todos", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newTodoId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newTodoId, 0);

    // 2. Чтение созданной задачи
    mockUserTodoService->setGetTodoResult(newTodo);
    auto getResponse = makeGetRequest("/api/v1/user-todos/" + std::to_string(newTodoId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление задачи
    dto::UserTodo updatedTodo = newTodo;
    updatedTodo.caption = "Обновлённый жизненный цикл";
    updatedTodo.isDone = true;
    mockUserTodoService->setUpdateTodoResult(updatedTodo);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновлённый жизненный цикл"));
    updateBody[U("isDone")] = web::json::value::boolean(true);

    auto updateResponse = makePutRequest("/api/v1/user-todos/" + std::to_string(newTodoId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Получение списка задач
    services::UserTodosPage listPage;
    listPage.todos = { updatedTodo };
    listPage.totalCount = 1;
    mockUserTodoService->setGetTodosResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/user-todos").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление задачи
    services::UserTodoResult deleteResult;
    deleteResult.success = true;
    mockUserTodoService->setDeleteTodoResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/user-todos/" + std::to_string(newTodoId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockUserTodoService->setGetTodoResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/user-todos/" + std::to_string(newTodoId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
