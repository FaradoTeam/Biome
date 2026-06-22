#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_service.h"
#include "tests/server_mocks/mock_item_user_state_service.h"
#include "tests/server_mocks/mock_state_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct ItemUserStatesTestFixture
{
    ItemUserStatesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemService = std::make_shared<MockItemService>();
        mockStateService = std::make_shared<MockStateService>();
        mockItemUserStateService = std::make_shared<MockItemUserStateService>();

        // Настраиваем мок ItemService для проверки доступа
        dto::Item testItem;
        testItem.id = 1;
        testItem.caption = "Test Item";
        testItem.itemTypeId = 1;
        testItem.stateId = 1;
        testItem.phaseId = 1;
        mockItemService->setGetItemResult(testItem);

        // Пользователь 999 не имеет доступа
        mockItemService->setGetItemResultForUser(999, std::nullopt);

        // Обычный пользователь (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultItemUserStateService();

        server = std::make_unique<RestServer>("127.0.0.1", 18102);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemService(mockItemService);
        server->setStateService(mockStateService);
        server->setItemUserStateService(mockItemUserStateService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemUserStateService()
    {
        // Настройка списка записей истории состояний
        services::ItemUserStatesPage testPage;

        dto::ItemUserState state1;
        state1.id = 1;
        state1.itemId = 1;
        state1.userId = 100;
        state1.stateId = 1;
        state1.comment = "Переход в состояние 1";
        state1.timestamp = std::chrono::system_clock::now();

        dto::ItemUserState state2;
        state2.id = 2;
        state2.itemId = 1;
        state2.userId = 100;
        state2.stateId = 2;
        state2.comment = "Переход в состояние 2";
        state2.timestamp = std::chrono::system_clock::now() + std::chrono::hours(1);

        dto::ItemUserState state3;
        state3.id = 3;
        state3.itemId = 2;
        state3.userId = 100;
        state3.stateId = 1;
        state3.comment = "Переход для другого элемента";
        state3.timestamp = std::chrono::system_clock::now();

        testPage.states = { state1, state2, state3 };
        testPage.totalCount = 3;
        mockItemUserStateService->setGetItemUserStatesResult(testPage);
        mockItemUserStateService->setGetItemUserStateResult(state1);

        dto::ItemUserState newState;
        newState.id = 100;
        newState.itemId = 1;
        newState.userId = 100;
        newState.stateId = 3;
        newState.comment = "Новая запись истории";
        mockItemUserStateService->setCreateItemUserStateResult(newState);

        // Для пользователя 999 (без прав) - возвращаем nullopt
        mockItemUserStateService->setCreateItemUserStateResultForUser(999, std::nullopt);

        services::ItemUserStateResult deleteResult;
        deleteResult.success = true;
        mockItemUserStateService->setDeleteItemUserStateResult(deleteResult);
    }

    ~ItemUserStatesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18102"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18102"));
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

    pplx::task<web::http::http_response> makeDeleteRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18102"));
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
    std::shared_ptr<MockItemService> mockItemService;
    std::shared_ptr<MockStateService> mockStateService;
    std::shared_ptr<MockItemUserStateService> mockItemUserStateService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemUserStatesCrudTestSuite, ItemUserStatesTestFixture)

// ============================================================
// GET /api/v1/items/user-states — Получение списка записей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_user_states_returns_list)
{
    auto response = makeGetRequest("/api/v1/items/user-states").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getGetItemUserStatesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetItemUserStatesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_user_states_with_pagination_params)
{
    services::ItemUserStatesPage emptyPage;
    mockItemUserStateService->setGetItemUserStatesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/items/user-states?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetItemUserStatesPage(), 3);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetItemUserStatesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_item_user_states_filter_by_item_id)
{
    services::ItemUserStatesPage filteredPage;
    dto::ItemUserState state;
    state.id = 10;
    state.itemId = 42;
    state.userId = 100;
    state.stateId = 1;
    filteredPage.states = { state };
    filteredPage.totalCount = 1;
    mockItemUserStateService->setGetItemUserStatesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/items/user-states?itemId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemUserStateService->getLastGetItemUserStatesItemId().has_value());
    BOOST_CHECK_EQUAL(*mockItemUserStateService->getLastGetItemUserStatesItemId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_item_user_states_filter_by_user_id)
{
    services::ItemUserStatesPage filteredPage;
    dto::ItemUserState state;
    state.id = 20;
    state.itemId = 1;
    state.userId = 200;
    state.stateId = 1;
    filteredPage.states = { state };
    filteredPage.totalCount = 1;
    mockItemUserStateService->setGetItemUserStatesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/items/user-states?userId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemUserStateService->getLastGetItemUserStatesFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockItemUserStateService->getLastGetItemUserStatesFilterUserId(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_item_user_states_requires_auth)
{
    auto response = makeGetRequest("/api/v1/items/user-states", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getGetItemUserStatesCallCount(), 0);
}

// ============================================================
// GET /api/v1/items/user-states/{id} — Получение записи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_user_state_by_id_success)
{
    dto::ItemUserState state;
    state.id = 42;
    state.itemId = 1;
    state.userId = 100;
    state.stateId = 2;
    state.comment = "Конкретная запись";
    mockItemUserStateService->setGetItemUserStateResult(state);

    auto response = makeGetRequest("/api/v1/items/user-states/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getGetItemUserStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetItemUserStateId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("stateId")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("comment")).as_string(), U("Конкретная запись"));
}

BOOST_AUTO_TEST_CASE(test_get_item_user_state_not_found)
{
    mockItemUserStateService->setGetItemUserStateResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/items/user-states/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getGetItemUserStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetItemUserStateId(), 999);
}

// ============================================================
// GET /api/v1/items/{itemId}/user-states/last — Последняя запись для элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_last_item_user_state_success)
{
    dto::ItemUserState lastState;
    lastState.id = 5;
    lastState.itemId = 1;
    lastState.userId = 100;
    lastState.stateId = 2;
    lastState.comment = "Последнее состояние";
    mockItemUserStateService->setGetLastItemUserStateResult(lastState);

    auto response = makeGetRequest("/api/v1/items/1/user-states/last").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getGetLastItemUserStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastGetLastItemUserStateItemId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("comment")).as_string(), U("Последнее состояние"));
}

BOOST_AUTO_TEST_CASE(test_get_last_item_user_state_not_found)
{
    mockItemUserStateService->setGetLastItemUserStateResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/items/999/user-states/last").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/items/{itemId}/user-states — Создание записи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_user_state_success)
{
    dto::ItemUserState createdState;
    createdState.id = 100;
    createdState.itemId = 1;
    createdState.userId = 100;
    createdState.stateId = 3;
    createdState.comment = "Новая запись истории";
    mockItemUserStateService->setCreateItemUserStateResult(createdState);

    web::json::value body;
    body[U("stateId")] = web::json::value::number(3);
    body[U("comment")] = web::json::value::string(U("Новая запись истории"));

    auto response = makePostRequest("/api/v1/items/1/user-states", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getCreateItemUserStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastCreateItemUserStateUserId(), 100);
    BOOST_CHECK_EQUAL(*mockItemUserStateService->getLastCreatedItemUserState().stateId, 3);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("stateId")).as_integer(), 3);
}

BOOST_AUTO_TEST_CASE(test_create_item_user_state_missing_state_id)
{
    web::json::value body;
    body[U("comment")] = web::json::value::string(U("Комментарий без stateId"));

    auto response = makePostRequest("/api/v1/items/1/user-states", body).get();

    // Отсутствует обязательное поле stateId - сервис вернёт nullopt
    // Обработчик вернёт 400 Bad Request
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

BOOST_AUTO_TEST_CASE(test_create_item_user_state_forbidden)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    web::json::value body;
    body[U("stateId")] = web::json::value::number(3);
    body[U("comment")] = web::json::value::string(U("Попытка создания"));

    auto response = makePostRequest("/api/v1/items/1/user-states", body).get();

    // Пользователь 999 не имеет прав - сервер возвращает 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/items/user-states/{id} — Удаление записи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_item_user_state_success)
{
    services::ItemUserStateResult deleteResult;
    deleteResult.success = true;
    mockItemUserStateService->setDeleteItemUserStateResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/items/user-states/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getDeleteItemUserStateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastDeletedItemUserStateId(), 3);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getLastDeleteItemUserStateUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_item_user_state_not_found)
{
    services::ItemUserStateResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "State not found";
    mockItemUserStateService->setDeleteItemUserStateResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/items/user-states/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getDeleteItemUserStateCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_item_user_state_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/items/user-states/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemUserStateService->getDeleteItemUserStateCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_user_state_lifecycle)
{
    // 1. Создание записи
    dto::ItemUserState newState;
    newState.id = 200;
    newState.itemId = 1;
    newState.userId = 100;
    newState.stateId = 3;
    newState.comment = "Жизненный цикл записи";
    mockItemUserStateService->setCreateItemUserStateResult(newState);

    web::json::value createBody;
    createBody[U("stateId")] = web::json::value::number(3);
    createBody[U("comment")] = web::json::value::string(U("Жизненный цикл записи"));

    auto createResponse = makePostRequest("/api/v1/items/1/user-states", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newStateId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newStateId, 0);

    // 2. Чтение созданной записи
    mockItemUserStateService->setGetItemUserStateResult(newState);
    auto getResponse = makeGetRequest("/api/v1/items/user-states/" + std::to_string(newStateId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение последней записи для элемента
    mockItemUserStateService->setGetLastItemUserStateResult(newState);
    auto getLastResponse = makeGetRequest("/api/v1/items/1/user-states/last").get();
    BOOST_CHECK_EQUAL(getLastResponse.status_code(), status_codes::OK);

    // 4. Получение списка записей
    services::ItemUserStatesPage listPage;
    listPage.states = { newState };
    listPage.totalCount = 1;
    mockItemUserStateService->setGetItemUserStatesResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/items/user-states?itemId=1").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление записи
    services::ItemUserStateResult deleteResult;
    deleteResult.success = true;
    mockItemUserStateService->setDeleteItemUserStateResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/items/user-states/" + std::to_string(newStateId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
