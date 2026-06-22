#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_link_service.h"
#include "tests/server_mocks/mock_item_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server::tests
{

struct ItemLinksTestFixture
{
    ItemLinksTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemService = std::make_shared<MockItemService>();
        mockItemLinkService = std::make_shared<MockItemLinkService>();

        // Обычный пользователь (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultItemLinkService();

        server = std::make_unique<RestServer>("127.0.0.1", 18111);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemService(mockItemService);
        server->setItemLinkService(mockItemLinkService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemLinkService()
    {
        // Настройка списка связей
        services::ItemLinksPage testPage;

        dto::ItemLink link1;
        link1.id = 1;
        link1.linkTypeId = 1;
        link1.sourceItemId = 100;
        link1.destinationItemId = 101;

        dto::ItemLink link2;
        link2.id = 2;
        link2.linkTypeId = 1;
        link2.sourceItemId = 100;
        link2.destinationItemId = 102;

        dto::ItemLink link3;
        link3.id = 3;
        link3.linkTypeId = 2;
        link3.sourceItemId = 101;
        link3.destinationItemId = 102;

        testPage.links = { link1, link2, link3 };
        testPage.totalCount = 3;
        mockItemLinkService->setGetItemLinksResult(testPage);
        mockItemLinkService->setGetItemLinkResult(link1);

        // Настройка моков для проверки доступа к элементам
        dto::Item item100;
        item100.id = 100;
        item100.caption = "Элемент 100";
        item100.itemTypeId = 1;
        item100.stateId = 1;
        item100.phaseId = 1;

        dto::Item item101;
        item101.id = 101;
        item101.caption = "Элемент 101";
        item101.itemTypeId = 1;
        item101.stateId = 1;
        item101.phaseId = 1;

        dto::Item item102;
        item102.id = 102;
        item102.caption = "Элемент 102";
        item102.itemTypeId = 1;
        item102.stateId = 1;
        item102.phaseId = 1;

        mockItemService->setGetItemResult(item100);
        mockItemService->setGetItemResult(item101);
        mockItemService->setGetItemResult(item102);

        // Для пользователя 999 (без прав)
        mockItemService->setGetItemResultForUser(999, std::nullopt);

        dto::ItemLink newLink;
        newLink.id = 100;
        newLink.linkTypeId = 1;
        newLink.sourceItemId = 100;
        newLink.destinationItemId = 103;
        mockItemLinkService->setCreateItemLinkResult(newLink);

        services::ItemLinkResult deleteResult;
        deleteResult.success = true;
        mockItemLinkService->setDeleteItemLinkResult(deleteResult);

        // Настройка связей по элементу
        std::vector<dto::ItemLink> linksByItem = { link1, link2 };
        mockItemLinkService->setItemLinksByItemIdResult(linksByItem);

        // Настройка связей по типу
        std::vector<dto::ItemLink> linksByType = { link1, link2 };
        mockItemLinkService->setItemLinksByLinkTypeIdResult(linksByType);
    }

    ~ItemLinksTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18111"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18111"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18111"));
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
    std::shared_ptr<MockItemLinkService> mockItemLinkService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemLinksCrudTestSuite, ItemLinksTestFixture)

// ============================================================
// GET /api/v1/item-links — Получение списка связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_links_returns_list)
{
    auto response = makeGetRequest("/api/v1/item-links").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinksCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinksUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_with_pagination_params)
{
    services::ItemLinksPage emptyPage;
    mockItemLinkService->setGetItemLinksResult(emptyPage);

    auto response = makeGetRequest("/api/v1/item-links?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinksPage(), 3);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinksPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_filter_by_link_type)
{
    services::ItemLinksPage filteredPage;
    dto::ItemLink link;
    link.id = 10;
    link.linkTypeId = 42;
    link.sourceItemId = 100;
    link.destinationItemId = 101;
    filteredPage.links = { link };
    filteredPage.totalCount = 1;
    mockItemLinkService->setGetItemLinksResult(filteredPage);

    auto response = makeGetRequest("/api/v1/item-links?linkTypeId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemLinkService->getLastGetItemLinksLinkTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockItemLinkService->getLastGetItemLinksLinkTypeId(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_filter_by_source_item)
{
    services::ItemLinksPage filteredPage;
    dto::ItemLink link;
    link.id = 20;
    link.linkTypeId = 1;
    link.sourceItemId = 200;
    link.destinationItemId = 101;
    filteredPage.links = { link };
    filteredPage.totalCount = 1;
    mockItemLinkService->setGetItemLinksResult(filteredPage);

    auto response = makeGetRequest("/api/v1/item-links?sourceItemId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemLinkService->getLastGetItemLinksSourceItemId().has_value());
    BOOST_CHECK_EQUAL(*mockItemLinkService->getLastGetItemLinksSourceItemId(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_filter_by_destination_item)
{
    services::ItemLinksPage filteredPage;
    dto::ItemLink link;
    link.id = 30;
    link.linkTypeId = 1;
    link.sourceItemId = 100;
    link.destinationItemId = 300;
    filteredPage.links = { link };
    filteredPage.totalCount = 1;
    mockItemLinkService->setGetItemLinksResult(filteredPage);

    auto response = makeGetRequest("/api/v1/item-links?destinationItemId=300").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemLinkService->getLastGetItemLinksDestItemId().has_value());
    BOOST_CHECK_EQUAL(*mockItemLinkService->getLastGetItemLinksDestItemId(), 300);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_empty_list)
{
    services::ItemLinksPage emptyPage;
    mockItemLinkService->setGetItemLinksResult(emptyPage);

    auto response = makeGetRequest("/api/v1/item-links").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_requires_auth)
{
    auto response = makeGetRequest("/api/v1/item-links", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinksCallCount(), 0);
}

// ============================================================
// GET /api/v1/item-links/{id} — Получение связи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_link_by_id_success)
{
    dto::ItemLink link;
    link.id = 42;
    link.linkTypeId = 1;
    link.sourceItemId = 100;
    link.destinationItemId = 101;
    mockItemLinkService->setGetItemLinkResult(link);

    auto response = makeGetRequest("/api/v1/item-links/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinkCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinkId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("linkTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("sourceItemId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("destinationItemId")).as_integer(), 101);
}

BOOST_AUTO_TEST_CASE(test_get_item_link_not_found)
{
    mockItemLinkService->setGetItemLinkResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/item-links/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinkCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinkId(), 999);
}

// ============================================================
// GET /api/v1/items/{itemId}/links — Получение связей элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_links_by_item_id_success)
{
    std::vector<dto::ItemLink> links;
    dto::ItemLink link1;
    link1.id = 1;
    link1.linkTypeId = 1;
    link1.sourceItemId = 100;
    link1.destinationItemId = 101;

    dto::ItemLink link2;
    link2.id = 2;
    link2.linkTypeId = 1;
    link2.sourceItemId = 100;
    link2.destinationItemId = 102;

    links = { link1, link2 };
    mockItemLinkService->setItemLinksByItemIdResult(links);

    auto response = makeGetRequest("/api/v1/items/100/links").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinksByItemIdCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinksByItemId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
    BOOST_CHECK_EQUAL(json[0].at(U("sourceItemId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_item_links_by_item_id_empty)
{
    mockItemLinkService->setItemLinksByItemIdResult({});

    auto response = makeGetRequest("/api/v1/items/999/links").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// GET /api/v1/link-types/{linkTypeId}/links — Получение связей по типу
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_links_by_link_type_id_success)
{
    std::vector<dto::ItemLink> links;
    dto::ItemLink link1;
    link1.id = 1;
    link1.linkTypeId = 5;
    link1.sourceItemId = 100;
    link1.destinationItemId = 101;

    dto::ItemLink link2;
    link2.id = 2;
    link2.linkTypeId = 5;
    link2.sourceItemId = 100;
    link2.destinationItemId = 102;

    links = { link1, link2 };
    mockItemLinkService->setItemLinksByLinkTypeIdResult(links);

    auto response = makeGetRequest("/api/v1/link-types/5/links").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemLinkService->getGetItemLinksByLinkTypeIdCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastGetItemLinksByLinkTypeId(), 5);
}

// ============================================================
// POST /api/v1/item-links — Создание связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_link_success)
{
    dto::ItemLink createdLink;
    createdLink.id = 100;
    createdLink.linkTypeId = 1;
    createdLink.sourceItemId = 100;
    createdLink.destinationItemId = 103;
    mockItemLinkService->setCreateItemLinkResult(createdLink);

    web::json::value body;
    body[U("linkTypeId")] = web::json::value::number(1);
    body[U("sourceItemId")] = web::json::value::number(100);
    body[U("destinationItemId")] = web::json::value::number(103);

    auto response = makePostRequest("/api/v1/item-links", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockItemLinkService->getCreateItemLinkCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastCreateItemLinkUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("linkTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("sourceItemId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("destinationItemId")).as_integer(), 103);
}

BOOST_AUTO_TEST_CASE(test_create_item_link_missing_required_fields)
{
    web::json::value body;
    body[U("linkTypeId")] = web::json::value::number(1);
    body[U("sourceItemId")] = web::json::value::number(100);

    auto response = makePostRequest("/api/v1/item-links", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockItemLinkService->getCreateItemLinkCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_item_link_duplicate)
{
    mockItemLinkService->setCreateItemLinkResult(std::nullopt);

    web::json::value body;
    body[U("linkTypeId")] = web::json::value::number(1);
    body[U("sourceItemId")] = web::json::value::number(100);
    body[U("destinationItemId")] = web::json::value::number(101);

    auto response = makePostRequest("/api/v1/item-links", body).get();

    // Существующая связь -> конфликт или недостаточно прав
    BOOST_CHECK(response.status_code() == status_codes::Forbidden || response.status_code() == status_codes::Conflict);
}

BOOST_AUTO_TEST_CASE(test_create_item_link_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockItemLinkService->setCreateItemLinkResult(std::nullopt);

    web::json::value body;
    body[U("linkTypeId")] = web::json::value::number(1);
    body[U("sourceItemId")] = web::json::value::number(100);
    body[U("destinationItemId")] = web::json::value::number(103);

    auto response = makePostRequest("/api/v1/item-links", body).get();

    BOOST_CHECK(response.status_code() == status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/item-links/{id} — Удаление связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_item_link_success)
{
    services::ItemLinkResult deleteResult;
    deleteResult.success = true;
    mockItemLinkService->setDeleteItemLinkResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/item-links/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockItemLinkService->getDeleteItemLinkCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastDeletedItemLinkId(), 3);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastDeleteItemLinkUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_item_link_not_found)
{
    services::ItemLinkResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Item link not found";
    mockItemLinkService->setDeleteItemLinkResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/item-links/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemLinkService->getDeleteItemLinkCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemLinkService->getLastDeletedItemLinkId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_item_link_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::ItemLinkResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    mockItemLinkService->setDeleteItemLinkResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/item-links/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_item_link_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/item-links/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemLinkService->getDeleteItemLinkCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_item_link_lifecycle)
{
    // 1. Создание связи
    dto::ItemLink newLink;
    newLink.id = 200;
    newLink.linkTypeId = 1;
    newLink.sourceItemId = 100;
    newLink.destinationItemId = 200;
    mockItemLinkService->setCreateItemLinkResult(newLink);

    web::json::value createBody;
    createBody[U("linkTypeId")] = web::json::value::number(1);
    createBody[U("sourceItemId")] = web::json::value::number(100);
    createBody[U("destinationItemId")] = web::json::value::number(200);

    auto createResponse = makePostRequest("/api/v1/item-links", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданной связи
    mockItemLinkService->setGetItemLinkResult(newLink);
    auto getResponse = makeGetRequest("/api/v1/item-links/200").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение связей элемента
    std::vector<dto::ItemLink> links = { newLink };
    mockItemLinkService->setItemLinksByItemIdResult(links);
    auto getByItemResponse = makeGetRequest("/api/v1/items/100/links").get();
    BOOST_CHECK_EQUAL(getByItemResponse.status_code(), status_codes::OK);

    // 4. Получение связей по типу
    mockItemLinkService->setItemLinksByLinkTypeIdResult(links);
    auto getByTypeResponse = makeGetRequest("/api/v1/link-types/1/links").get();
    BOOST_CHECK_EQUAL(getByTypeResponse.status_code(), status_codes::OK);

    // 5. Удаление связи
    services::ItemLinkResult deleteResult;
    deleteResult.success = true;
    mockItemLinkService->setDeleteItemLinkResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/item-links/200").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockItemLinkService->setGetItemLinkResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/item-links/200").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
