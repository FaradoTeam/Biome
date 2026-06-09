#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_history_service.h"
#include "tests/server_mocks/mock_item_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct ItemHistoriesTestFixture
{
    ItemHistoriesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemService = std::make_shared<MockItemService>();
        mockItemHistoryService = std::make_shared<MockItemHistoryService>();

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
        setupDefaultItemHistoryService();

        server = std::make_unique<RestServer>("127.0.0.1", 18103);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemService(mockItemService);
        server->setItemHistoryService(mockItemHistoryService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemHistoryService()
    {
        // Настройка списка записей истории изменений
        services::ItemHistoriesPage testPage;

        dto::ItemHistory history1;
        history1.id = 1;
        history1.itemId = 1;
        history1.userId = 100;
        history1.diff = R"({"caption": "Изменён заголовок"})";
        history1.timestamp = std::chrono::system_clock::now();

        dto::ItemHistory history2;
        history2.id = 2;
        history2.itemId = 1;
        history2.userId = 100;
        history2.diff = R"({"content": "Обновлено содержимое"})";
        history2.timestamp = std::chrono::system_clock::now() + std::chrono::hours(1);

        dto::ItemHistory history3;
        history3.id = 3;
        history3.itemId = 2;
        history3.userId = 100;
        history3.diff = R"({"stateId": 2})";
        history3.timestamp = std::chrono::system_clock::now();

        testPage.histories = { history1, history2, history3 };
        testPage.totalCount = 3;
        mockItemHistoryService->setGetItemHistoriesResult(testPage);
        mockItemHistoryService->setGetItemHistoryResult(history1);

        dto::ItemHistory newHistory;
        newHistory.id = 100;
        newHistory.itemId = 1;
        newHistory.userId = 100;
        newHistory.diff = R"({"caption": "Новая запись истории"})";
        mockItemHistoryService->setCreateItemHistoryResult(newHistory);

        // Для пользователя 999 (без прав) - возвращаем nullopt
        mockItemHistoryService->setCreateItemHistoryResultForUser(999, std::nullopt);

        services::ItemHistoryResult deleteResult;
        deleteResult.success = true;
        mockItemHistoryService->setDeleteItemHistoryResult(deleteResult);
    }

    ~ItemHistoriesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18103"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18103"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18103"));
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
    std::shared_ptr<MockItemHistoryService> mockItemHistoryService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemHistoriesCrudTestSuite, ItemHistoriesTestFixture)

// ============================================================
// GET /api/items/histories — Получение списка записей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_histories_returns_list)
{
    auto response = makeGetRequest("/api/items/histories").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getGetItemHistoriesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetItemHistoriesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_histories_with_pagination_params)
{
    services::ItemHistoriesPage emptyPage;
    mockItemHistoryService->setGetItemHistoriesResult(emptyPage);

    auto response = makeGetRequest("/api/items/histories?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetItemHistoriesPage(), 3);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetItemHistoriesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_item_histories_filter_by_item_id)
{
    services::ItemHistoriesPage filteredPage;
    dto::ItemHistory history;
    history.id = 10;
    history.itemId = 42;
    history.userId = 100;
    history.diff = R"({"test": "value"})";
    filteredPage.histories = { history };
    filteredPage.totalCount = 1;
    mockItemHistoryService->setGetItemHistoriesResult(filteredPage);

    auto response = makeGetRequest("/api/items/histories?itemId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemHistoryService->getLastGetItemHistoriesItemId().has_value());
    BOOST_CHECK_EQUAL(*mockItemHistoryService->getLastGetItemHistoriesItemId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_item_histories_filter_by_user_id)
{
    services::ItemHistoriesPage filteredPage;
    dto::ItemHistory history;
    history.id = 20;
    history.itemId = 1;
    history.userId = 200;
    history.diff = R"({"test": "value"})";
    filteredPage.histories = { history };
    filteredPage.totalCount = 1;
    mockItemHistoryService->setGetItemHistoriesResult(filteredPage);

    auto response = makeGetRequest("/api/items/histories?userId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemHistoryService->getLastGetItemHistoriesFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockItemHistoryService->getLastGetItemHistoriesFilterUserId(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_item_histories_filter_by_date_range)
{
    services::ItemHistoriesPage filteredPage;
    dto::ItemHistory history;
    history.id = 30;
    history.itemId = 1;
    history.userId = 100;
    history.diff = R"({"test": "value"})";
    filteredPage.histories = { history };
    filteredPage.totalCount = 1;
    mockItemHistoryService->setGetItemHistoriesResult(filteredPage);

    auto response = makeGetRequest("/api/items/histories?dateFrom=1609459200&dateTo=1700000000").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemHistoryService->getLastGetItemHistoriesDateFrom().has_value());
}

BOOST_AUTO_TEST_CASE(test_get_item_histories_requires_auth)
{
    auto response = makeGetRequest("/api/items/histories", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getGetItemHistoriesCallCount(), 0);
}

// ============================================================
// GET /api/items/histories/{id} — Получение записи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_history_by_id_success)
{
    dto::ItemHistory history;
    history.id = 42;
    history.itemId = 1;
    history.userId = 100;
    history.diff = R"({"caption": "Конкретное изменение"})";
    mockItemHistoryService->setGetItemHistoryResult(history);

    auto response = makeGetRequest("/api/items/histories/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getGetItemHistoryCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetItemHistoryId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
    BOOST_CHECK(json.has_field(U("diff")));
}

BOOST_AUTO_TEST_CASE(test_get_item_history_not_found)
{
    mockItemHistoryService->setGetItemHistoryResult(std::nullopt);

    auto response = makeGetRequest("/api/items/histories/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getGetItemHistoryCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetItemHistoryId(), 999);
}

// ============================================================
// GET /api/items/{itemId}/histories/last — Последняя запись для элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_last_item_history_success)
{
    dto::ItemHistory lastHistory;
    lastHistory.id = 5;
    lastHistory.itemId = 1;
    lastHistory.userId = 100;
    lastHistory.diff = R"({"stateId": 2})";
    mockItemHistoryService->setGetLastItemHistoryResult(lastHistory);

    auto response = makeGetRequest("/api/items/1/histories/last").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getGetLastItemHistoryCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastGetLastItemHistoryItemId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_last_item_history_not_found)
{
    mockItemHistoryService->setGetLastItemHistoryResult(std::nullopt);

    auto response = makeGetRequest("/api/items/999/histories/last").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/items/{itemId}/histories — Создание записи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_history_success)
{
    dto::ItemHistory createdHistory;
    createdHistory.id = 100;
    createdHistory.itemId = 1;
    createdHistory.userId = 100;
    createdHistory.diff = R"({"caption": "Новая запись истории"})";
    mockItemHistoryService->setCreateItemHistoryResult(createdHistory);

    web::json::value body;
    body[U("diff")] = web::json::value::string(U("{\"caption\": \"Новая запись истории\"}"));

    auto response = makePostRequest("/api/items/1/histories", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getCreateItemHistoryCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastCreateItemHistoryUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_item_history_empty_body)
{
    web::json::value body = web::json::value::object();

    auto response = makePostRequest("/api/items/1/histories", body).get();

    // Пустой объект {} - валидный JSON, создаётся запись с пустым diff
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
}

BOOST_AUTO_TEST_CASE(test_create_item_history_forbidden)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    web::json::value body;
    body[U("diff")] = web::json::value::string(U("{\"test\": \"value\"}"));

    auto response = makePostRequest("/api/items/1/histories", body).get();

    // Пользователь 999 не имеет прав - сервер возвращает 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/items/histories/{id} — Удаление записи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_item_history_success)
{
    services::ItemHistoryResult deleteResult;
    deleteResult.success = true;
    mockItemHistoryService->setDeleteItemHistoryResult(deleteResult);

    auto response = makeDeleteRequest("/api/items/histories/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getDeleteItemHistoryCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastDeletedItemHistoryId(), 3);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getLastDeleteItemHistoryUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_item_history_not_found)
{
    services::ItemHistoryResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "History not found";
    mockItemHistoryService->setDeleteItemHistoryResult(deleteResult);

    auto response = makeDeleteRequest("/api/items/histories/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getDeleteItemHistoryCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_item_history_requires_auth)
{
    auto response = makeDeleteRequest("/api/items/histories/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemHistoryService->getDeleteItemHistoryCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_history_lifecycle)
{
    // 1. Создание записи
    dto::ItemHistory newHistory;
    newHistory.id = 200;
    newHistory.itemId = 1;
    newHistory.userId = 100;
    newHistory.diff = R"({"caption": "Жизненный цикл истории"})";
    mockItemHistoryService->setCreateItemHistoryResult(newHistory);

    web::json::value createBody;
    createBody[U("diff")] = web::json::value::string(U("{\"caption\": \"Жизненный цикл истории\"}"));

    auto createResponse = makePostRequest("/api/items/1/histories", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newHistoryId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newHistoryId, 0);

    // 2. Чтение созданной записи
    mockItemHistoryService->setGetItemHistoryResult(newHistory);
    auto getResponse = makeGetRequest("/api/items/histories/" + std::to_string(newHistoryId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение последней записи для элемента
    mockItemHistoryService->setGetLastItemHistoryResult(newHistory);
    auto getLastResponse = makeGetRequest("/api/items/1/histories/last").get();
    BOOST_CHECK_EQUAL(getLastResponse.status_code(), status_codes::OK);

    // 4. Получение списка записей
    services::ItemHistoriesPage listPage;
    listPage.histories = { newHistory };
    listPage.totalCount = 1;
    mockItemHistoryService->setGetItemHistoriesResult(listPage);
    auto listResponse = makeGetRequest("/api/items/histories?itemId=1").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление записи
    services::ItemHistoryResult deleteResult;
    deleteResult.success = true;
    mockItemHistoryService->setDeleteItemHistoryResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/items/histories/" + std::to_string(newHistoryId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
