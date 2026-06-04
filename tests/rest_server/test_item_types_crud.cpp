#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_type_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct ItemTypesTestFixture
{
    ItemTypesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemTypeService = std::make_shared<MockItemTypeService>();

        // Супер-админ (userId=1) для создания/обновления/удаления типов элементов
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultItemTypeService();

        server = std::make_unique<RestServer>("127.0.0.1", 18086);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemTypeService(mockItemTypeService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemTypeService()
    {
        // Настройка списка типов элементов
        services::ItemTypesPage testPage;

        dto::ItemType issueType;
        issueType.id = 1;
        issueType.caption = "Задача";
        issueType.kind = "issue";
        issueType.workflowId = 1;
        issueType.defaultStateId = 1;
        issueType.defaultContent = "## Описание задачи\n\n";

        dto::ItemType folderType;
        folderType.id = 2;
        folderType.caption = "Папка";
        folderType.kind = "folder";
        folderType.workflowId = 1;
        folderType.defaultStateId = 1;

        dto::ItemType testCaseType;
        testCaseType.id = 3;
        testCaseType.caption = "Тест-кейс";
        testCaseType.kind = "test-case";
        testCaseType.workflowId = 1;
        testCaseType.defaultStateId = 1;
        testCaseType.defaultContent = "## Предусловия\n\n## Шаги\n\n## Ожидаемый результат";

        testPage.itemTypes = { issueType, folderType, testCaseType };
        testPage.totalCount = 3;
        mockItemTypeService->setGetItemTypesResult(testPage);
        mockItemTypeService->setGetItemTypeResult(issueType);

        dto::ItemType newItemType;
        newItemType.id = 4;
        newItemType.caption = "Требование";
        newItemType.kind = "requirement";
        newItemType.workflowId = 1;
        newItemType.defaultStateId = 1;
        mockItemTypeService->setCreateItemTypeResult(newItemType);

        dto::ItemType updatedItemType = issueType;
        updatedItemType.caption = "Задача (обновленная)";
        mockItemTypeService->setUpdateItemTypeResult(updatedItemType);
        mockItemTypeService->setDeleteItemTypeResult(true);

        std::vector<dto::ItemType> workflowTypes = { issueType, folderType };
        mockItemTypeService->setItemTypesByWorkflowResult(workflowTypes);
    }

    ~ItemTypesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18086"));
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
    std::shared_ptr<MockItemTypeService> mockItemTypeService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemTypesCrudTestSuite, ItemTypesTestFixture)

// ============================================================
// GET /api/item-types — Получение списка типов элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_types_returns_list)
{
    auto response = makeGetRequest("/api/item-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->getItemTypesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_types_with_pagination_params)
{
    services::ItemTypesPage emptyPage;
    mockItemTypeService->setGetItemTypesResult(emptyPage);

    auto response = makeGetRequest("/api/item-types?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastGetItemTypesPage(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_types_with_workflow_filter)
{
    services::ItemTypesPage filteredPage;
    dto::ItemType itemType;
    itemType.id = 10;
    itemType.caption = "FilteredType";
    itemType.kind = "issue";
    itemType.workflowId = 42;
    filteredPage.itemTypes = { itemType };
    filteredPage.totalCount = 1;
    mockItemTypeService->setGetItemTypesResult(filteredPage);

    auto response = makeGetRequest("/api/item-types?workflowId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->getItemTypesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_item_types_with_kind_filter)
{
    services::ItemTypesPage filteredPage;
    dto::ItemType itemType;
    itemType.id = 20;
    itemType.caption = "IssueType";
    itemType.kind = "issue";
    filteredPage.itemTypes = { itemType };
    filteredPage.totalCount = 1;
    mockItemTypeService->setGetItemTypesResult(filteredPage);

    auto response = makeGetRequest("/api/item-types?kind=issue").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_item_types_empty_list)
{
    services::ItemTypesPage emptyPage;
    mockItemTypeService->setGetItemTypesResult(emptyPage);

    auto response = makeGetRequest("/api/item-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_item_types_requires_auth)
{
    auto response = makeGetRequest("/api/item-types", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemTypeService->getItemTypesCallCount(), 0);
}

// ============================================================
// GET /api/item-types/{id} — Получение типа элемента по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_type_by_id_success)
{
    dto::ItemType itemType;
    itemType.id = 42;
    itemType.caption = "Конкретный тип";
    itemType.kind = "requirement";
    itemType.workflowId = 1;
    mockItemTypeService->setGetItemTypeResult(itemType);

    auto response = makeGetRequest("/api/item-types/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->getItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastGetItemTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретный тип"));
    BOOST_CHECK_EQUAL(json.at(U("kind")).as_string(), U("requirement"));
}

BOOST_AUTO_TEST_CASE(test_get_item_type_not_found)
{
    mockItemTypeService->setGetItemTypeResult(std::nullopt);

    auto response = makeGetRequest("/api/item-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemTypeService->getItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastGetItemTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_item_type_invalid_id)
{
    auto response = makeGetRequest("/api/item-types/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_item_type_negative_id)
{
    auto response = makeGetRequest("/api/item-types/-1").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/item-types — Создание типа элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_type_success)
{
    dto::ItemType createdType;
    createdType.id = 100;
    createdType.caption = "Новый тип";
    createdType.kind = "folder";
    createdType.workflowId = 1;
    createdType.defaultStateId = 1;
    createdType.defaultContent = "Содержимое по умолчанию";
    mockItemTypeService->setCreateItemTypeResult(createdType);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый тип"));
    body[U("kind")] = web::json::value::string(U("folder"));
    body[U("workflowId")] = web::json::value::number(1);
    body[U("defaultStateId")] = web::json::value::number(1);
    body[U("defaultContent")] = web::json::value::string(U("Содержимое по умолчанию"));

    auto response = makePostRequest("/api/item-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockItemTypeService->createItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockItemTypeService->getLastCreatedItemType().caption,
        "Новый тип"
    );
    BOOST_CHECK_EQUAL(
        *mockItemTypeService->getLastCreatedItemType().kind,
        "folder"
    );
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastCreateItemTypeUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новый тип"));
    BOOST_CHECK_EQUAL(json.at(U("kind")).as_string(), U("folder"));
}

BOOST_AUTO_TEST_CASE(test_create_item_type_missing_required_fields)
{
    mockItemTypeService->setCreateItemTypeResult(std::nullopt);

    web::json::value body;
    body[U("kind")] = web::json::value::string(U("issue"));

    auto response = makePostRequest("/api/item-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockItemTypeService->createItemTypeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_item_type_with_all_kinds)
{
    std::vector<std::pair<std::string, std::string>> kindPairs = {
        { "issue", "Задача" },
        { "folder", "Папка" },
        { "test-case", "Тест-кейс" },
        { "test-cycle", "Тест-цикл" },
        { "test-execution", "Тест-прогон" },
        { "requirement", "Требование" }
    };

    for (size_t i = 0; i < kindPairs.size(); ++i)
    {
        dto::ItemType createdType;
        createdType.id = 200 + i;
        createdType.caption = kindPairs[i].second;
        createdType.kind = kindPairs[i].first;
        createdType.workflowId = 1;
        mockItemTypeService->setCreateItemTypeResult(createdType);

        web::json::value body;
        body[U("caption")] = web::json::value::string(
            U(kindPairs[i].second)
        );
        body[U("kind")] = web::json::value::string(
            U(kindPairs[i].first)
        );
        body[U("workflowId")] = web::json::value::number(1);

        auto response = makePostRequest("/api/item-types", body).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    }
}

// ============================================================
// PUT /api/item-types/{id} — Обновление типа элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_item_type_success)
{
    dto::ItemType updatedType;
    updatedType.id = 1;
    updatedType.caption = "Обновленный тип";
    updatedType.kind = "requirement";
    mockItemTypeService->setUpdateItemTypeResult(updatedType);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновленный тип"));
    body[U("kind")] = web::json::value::string(U("requirement"));

    auto response = makePutRequest("/api/item-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->updateItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockItemTypeService->getLastUpdatedItemType().id,
        1
    );
    BOOST_CHECK_EQUAL(
        *mockItemTypeService->getLastUpdatedItemType().caption,
        "Обновленный тип"
    );
    BOOST_CHECK_EQUAL(
        *mockItemTypeService->getLastUpdatedItemType().kind,
        "requirement"
    );
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastUpdateItemTypeUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_item_type_partial)
{
    dto::ItemType updatedType;
    updatedType.id = 1;
    updatedType.caption = "Только название";
    mockItemTypeService->setUpdateItemTypeResult(updatedType);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Только название"));

    auto response = makePutRequest("/api/item-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemTypeService->updateItemTypeCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_item_type_not_found)
{
    mockItemTypeService->setUpdateItemTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующий"));

    auto response = makePutRequest("/api/item-types/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemTypeService->updateItemTypeCallCount(), 1);
}

// ============================================================
// DELETE /api/item-types/{id} — Удаление типа элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_item_type_success)
{
    mockItemTypeService->setDeleteItemTypeResult(true);

    auto response = makeDeleteRequest("/api/item-types/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockItemTypeService->deleteItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastDeletedItemTypeId(), 2);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastDeleteItemTypeUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_item_type_not_found)
{
    mockItemTypeService->setDeleteItemTypeResult(false);

    auto response = makeDeleteRequest("/api/item-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemTypeService->deleteItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemTypeService->getLastDeletedItemTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_item_type_requires_auth)
{
    auto response = makeDeleteRequest("/api/item-types/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemTypeService->deleteItemTypeCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_type_lifecycle)
{
    // 1. Создание типа элемента
    dto::ItemType newType;
    newType.id = 100;
    newType.caption = "Жизненный цикл";
    newType.kind = "issue";
    newType.workflowId = 1;
    mockItemTypeService->setCreateItemTypeResult(newType);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл"));
    createBody[U("kind")] = web::json::value::string(U("issue"));
    createBody[U("workflowId")] = web::json::value::number(1);

    auto createResponse = makePostRequest("/api/item-types", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданного типа
    mockItemTypeService->setGetItemTypeResult(newType);
    auto getResponse = makeGetRequest("/api/item-types/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление типа
    dto::ItemType updatedType = newType;
    updatedType.caption = "Обновленный жизненный цикл";
    updatedType.kind = "requirement";
    mockItemTypeService->setUpdateItemTypeResult(updatedType);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(
        U("Обновленный жизненный цикл")
    );
    updateBody[U("kind")] = web::json::value::string(U("requirement"));

    auto updateResponse = makePutRequest("/api/item-types/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление типа
    mockItemTypeService->setDeleteItemTypeResult(true);
    auto deleteResponse = makeDeleteRequest("/api/item-types/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что тип удален
    mockItemTypeService->setGetItemTypeResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/item-types/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
