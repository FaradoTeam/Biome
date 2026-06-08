#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct ItemsTestFixture
{
    ItemsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemService = std::make_shared<MockItemService>();

        // Обычный пользователь (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultItemService();

        server = std::make_unique<RestServer>("127.0.0.1", 18100);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemService(mockItemService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemService()
    {
        // Настройка списка элементов
        services::ItemsPage testPage;

        dto::Item item1;
        item1.id = 1;
        item1.caption = "Задача 1";
        item1.itemTypeId = 1;
        item1.stateId = 1;
        item1.phaseId = 1;
        item1.content = "Содержимое задачи 1";
        item1.isDeleted = false;

        dto::Item item2;
        item2.id = 2;
        item2.caption = "Задача 2";
        item2.itemTypeId = 1;
        item2.stateId = 2;
        item2.phaseId = 1;
        item2.content = "Содержимое задачи 2";
        item2.isDeleted = false;

        dto::Item item3;
        item3.id = 3;
        item3.caption = "Удалённая задача";
        item3.itemTypeId = 1;
        item3.stateId = 3;
        item3.phaseId = 1;
        item3.content = "Содержимое удалённой задачи";
        item3.isDeleted = true;

        testPage.items = { item1, item2, item3 };
        testPage.totalCount = 3;
        mockItemService->setGetItemsResult(testPage);
        mockItemService->setGetItemResult(item1);

        dto::Item newItem;
        newItem.id = 100;
        newItem.caption = "Новая задача";
        newItem.itemTypeId = 1;
        newItem.stateId = 1;
        newItem.phaseId = 1;
        newItem.content = "Содержимое новой задачи";
        mockItemService->setCreateItemResult(newItem);

        dto::Item updatedItem = item1;
        updatedItem.caption = "Обновлённая задача";
        updatedItem.content = "Обновлённое содержимое";
        mockItemService->setUpdateItemResult(updatedItem);

        services::ItemResult deleteResult;
        deleteResult.success = true;
        mockItemService->setDeleteItemResult(deleteResult);

        services::ItemResult restoreResult;
        restoreResult.success = true;
        mockItemService->setRestoreItemResult(restoreResult);

        // Настройка полей элемента - оборачиваем в std::optional
        dto::ItemField field1;
        field1.id = 1;
        field1.itemId = 1;
        field1.fieldTypeId = 1;
        field1.value = "Высокий";

        dto::ItemField field2;
        field2.id = 2;
        field2.itemId = 1;
        field2.fieldTypeId = 2;
        field2.value = "В работе";

        std::vector<dto::ItemField> fields = { field1, field2 };
        mockItemService->setGetItemFieldsResult(fields);
        mockItemService->setGetItemFieldResult(field1);

        dto::ItemField newField;
        newField.id = 100;
        newField.itemId = 1;
        newField.fieldTypeId = 3;
        newField.value = "Новое значение";
        mockItemService->setSetItemFieldResult(newField);

        services::ItemResult deleteFieldResult;
        deleteFieldResult.success = true;
        mockItemService->setDeleteItemFieldResult(deleteFieldResult);
    }

    ~ItemsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18100"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18100"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18100"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18100"));
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
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemsCrudTestSuite, ItemsTestFixture)

// ============================================================
// GET /api/items — Получение списка элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_items_returns_list)
{
    auto response = makeGetRequest("/api/items").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemService->getGetItemsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemService->getLastGetItemsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_items_with_pagination_params)
{
    services::ItemsPage emptyPage;
    mockItemService->setGetItemsResult(emptyPage);

    auto response = makeGetRequest("/api/items?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemService->getLastGetItemsPage(), 3);
    BOOST_CHECK_EQUAL(mockItemService->getLastGetItemsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_items_filter_by_item_type)
{
    services::ItemsPage filteredPage;
    dto::Item item;
    item.id = 10;
    item.caption = "Filtered Item";
    item.itemTypeId = 42;
    filteredPage.items = { item };
    filteredPage.totalCount = 1;
    mockItemService->setGetItemsResult(filteredPage);

    auto response = makeGetRequest("/api/items?itemTypeId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemService->getLastGetItemsItemTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockItemService->getLastGetItemsItemTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemTypeId")).as_integer(), 42);
}

// ... остальные тесты аналогичны предыдущей версии ...

// ============================================================
// GET /api/items/{id}/fields — Получение полей элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_fields_success)
{
    std::vector<dto::ItemField> fields;
    dto::ItemField field1;
    field1.id = 1;
    field1.itemId = 1;
    field1.fieldTypeId = 1;
    field1.value = "Высокий";

    dto::ItemField field2;
    field2.id = 2;
    field2.itemId = 1;
    field2.fieldTypeId = 2;
    field2.value = "В работе";

    fields = { field1, field2 };
    mockItemService->setGetItemFieldsResult(fields);

    auto response = makeGetRequest("/api/items/1/fields").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemService->getGetItemFieldsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemService->getLastGetItemFieldsItemId(), 1);
    BOOST_CHECK_EQUAL(mockItemService->getLastGetItemFieldsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
    BOOST_CHECK_EQUAL(json[0].at(U("fieldTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json[1].at(U("fieldTypeId")).as_integer(), 2);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_lifecycle)
{
    // 1. Создание элемента
    dto::Item newItem;
    newItem.id = 200;
    newItem.caption = "Жизненный цикл элемента";
    newItem.itemTypeId = 1;
    newItem.stateId = 1;
    newItem.phaseId = 1;
    mockItemService->setCreateItemResult(newItem);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл элемента"));
    createBody[U("itemTypeId")] = web::json::value::number(1);
    createBody[U("stateId")] = web::json::value::number(1);
    createBody[U("phaseId")] = web::json::value::number(1);

    auto createResponse = makePostRequest("/api/items", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newItemId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newItemId, 0);

    // 2. Чтение созданного элемента
    mockItemService->setGetItemResult(newItem);
    auto getResponse = makeGetRequest("/api/items/" + std::to_string(newItemId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление элемента
    dto::Item updatedItem = newItem;
    updatedItem.caption = "Обновлённый элемент";
    mockItemService->setUpdateItemResult(updatedItem);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновлённый элемент"));

    auto updateResponse = makePutRequest("/api/items/" + std::to_string(newItemId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Установка поля
    dto::ItemField newField;
    newField.id = 300;
    newField.itemId = newItemId;
    newField.fieldTypeId = 10;
    newField.value = "Тестовое значение";
    mockItemService->setSetItemFieldResult(newField);

    web::json::value fieldBody;
    fieldBody[U("value")] = web::json::value::string(U("Тестовое значение"));

    auto setFieldResponse = makePutRequest(
                                "/api/items/" + std::to_string(newItemId) + "/fields/10", fieldBody
    )
                                .get();
    BOOST_CHECK_EQUAL(setFieldResponse.status_code(), status_codes::OK);

    // 5. Получение полей элемента
    std::vector<dto::ItemField> fields = { newField };
    mockItemService->setGetItemFieldsResult(fields);
    auto getFieldsResponse = makeGetRequest(
                                 "/api/items/" + std::to_string(newItemId) + "/fields"
    )
                                 .get();
    BOOST_CHECK_EQUAL(getFieldsResponse.status_code(), status_codes::OK);

    // 6. Удаление поля
    services::ItemResult deleteFieldResult;
    deleteFieldResult.success = true;
    mockItemService->setDeleteItemFieldResult(deleteFieldResult);

    auto deleteFieldResponse = makeDeleteRequest(
                                   "/api/items/" + std::to_string(newItemId) + "/fields/10"
    )
                                   .get();
    BOOST_CHECK_EQUAL(deleteFieldResponse.status_code(), status_codes::NoContent);

    // 7. Удаление элемента
    services::ItemResult deleteResult;
    deleteResult.success = true;
    mockItemService->setDeleteItemResult(deleteResult);

    auto deleteResponse = makeDeleteRequest("/api/items/" + std::to_string(newItemId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 8. Восстановление элемента
    services::ItemResult restoreResult;
    restoreResult.success = true;
    mockItemService->setRestoreItemResult(restoreResult);

    auto restoreResponse = makePostRequest(
                               "/api/items/" + std::to_string(newItemId) + "/restore", web::json::value::null()
    )
                               .get();
    BOOST_CHECK_EQUAL(restoreResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
