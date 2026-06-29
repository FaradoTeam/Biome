#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_item_document_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct ItemDocumentsTestFixture
{
    ItemDocumentsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockItemDocumentService = std::make_shared<MockItemDocumentService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultItemDocumentService();

        server = std::make_unique<RestServer>("127.0.0.1", 18123);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setItemDocumentService(mockItemDocumentService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultItemDocumentService()
    {
        // Настройка списка связей
        services::ItemDocumentsPage testPage;

        dto::ItemDocument link1;
        link1.id = 1;
        link1.itemId = 10;
        link1.documentId = 100;

        dto::ItemDocument link2;
        link2.id = 2;
        link2.itemId = 10;
        link2.documentId = 101;

        dto::ItemDocument link3;
        link3.id = 3;
        link3.itemId = 20;
        link3.documentId = 100;

        testPage.items = { link1, link2, link3 };
        testPage.totalCount = 3;
        mockItemDocumentService->setGetItemDocumentsResult(testPage);
        mockItemDocumentService->setGetItemDocumentResult(link1);

        dto::ItemDocument newLink;
        newLink.id = 100;
        newLink.itemId = 30;
        newLink.documentId = 102;
        mockItemDocumentService->setCreateItemDocumentResult(newLink);

        services::ItemDocumentResult deleteResult;
        deleteResult.success = true;
        mockItemDocumentService->setDeleteItemDocumentResult(deleteResult);

        // Документы по элементу
        std::vector<dto::ItemDocument> itemDocs = { link1, link2 };
        mockItemDocumentService->setGetDocumentsByItemResult(itemDocs);

        // Элементы по документу
        std::vector<dto::ItemDocument> docItems = { link1, link3 };
        mockItemDocumentService->setGetItemsByDocumentResult(docItems);
    }

    ~ItemDocumentsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18123"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18123"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18123"));
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
    std::shared_ptr<MockItemDocumentService> mockItemDocumentService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ItemDocumentsCrudTestSuite, ItemDocumentsTestFixture)

// ============================================================
// GET /api/v1/item-documents — Получение списка связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_documents_returns_list)
{
    auto response = makeGetRequest("/api/v1/item-documents").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetItemDocumentsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemDocumentsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_item_documents_with_pagination_params)
{
    services::ItemDocumentsPage emptyPage;
    mockItemDocumentService->setGetItemDocumentsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/item-documents?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemDocumentsPage(), 3);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemDocumentsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_item_documents_filter_by_item)
{
    services::ItemDocumentsPage filteredPage;
    dto::ItemDocument link;
    link.id = 10;
    link.itemId = 42;
    link.documentId = 100;
    filteredPage.items = { link };
    filteredPage.totalCount = 1;
    mockItemDocumentService->setGetItemDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/item-documents?itemId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemDocumentService->getLastGetItemDocumentsItemId().has_value());
    BOOST_CHECK_EQUAL(*mockItemDocumentService->getLastGetItemDocumentsItemId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_item_documents_filter_by_document)
{
    services::ItemDocumentsPage filteredPage;
    dto::ItemDocument link;
    link.id = 20;
    link.itemId = 10;
    link.documentId = 101;
    filteredPage.items = { link };
    filteredPage.totalCount = 1;
    mockItemDocumentService->setGetItemDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/item-documents?documentId=101").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockItemDocumentService->getLastGetItemDocumentsDocumentId().has_value());
    BOOST_CHECK_EQUAL(*mockItemDocumentService->getLastGetItemDocumentsDocumentId(), 101);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("documentId")).as_integer(), 101);
}

BOOST_AUTO_TEST_CASE(test_get_item_documents_requires_auth)
{
    auto response = makeGetRequest("/api/v1/item-documents", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetItemDocumentsCallCount(), 0);
}

// ============================================================
// GET /api/v1/item-documents/{id} — Получение связи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_item_document_by_id_success)
{
    dto::ItemDocument link;
    link.id = 42;
    link.itemId = 10;
    link.documentId = 100;
    mockItemDocumentService->setGetItemDocumentResult(link);

    auto response = makeGetRequest("/api/v1/item-documents/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetItemDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemDocumentId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("documentId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_item_document_not_found)
{
    mockItemDocumentService->setGetItemDocumentResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/item-documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetItemDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemDocumentId(), 999);
}

// ============================================================
// GET /api/v1/items/{itemId}/documents — Документы элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_documents_by_item_success)
{
    auto response = makeGetRequest("/api/v1/items/10/documents").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetDocumentsByItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetDocumentsByItemId(), 10);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

// ============================================================
// GET /api/v1/documents/{documentId}/items — Элементы документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_items_by_document_success)
{
    auto response = makeGetRequest("/api/v1/documents/100/items").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getGetItemsByDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastGetItemsByDocumentId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

// ============================================================
// POST /api/v1/item-documents — Создание связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_item_document_success)
{
    dto::ItemDocument createdLink;
    createdLink.id = 100;
    createdLink.itemId = 30;
    createdLink.documentId = 102;
    mockItemDocumentService->setCreateItemDocumentResult(createdLink);

    web::json::value body;
    body[U("itemId")] = web::json::value::number(30);
    body[U("documentId")] = web::json::value::number(102);

    auto response = makePostRequest("/api/v1/item-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getCreateItemDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastCreateItemDocumentUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 30);
    BOOST_CHECK_EQUAL(json.at(U("documentId")).as_integer(), 102);
}

BOOST_AUTO_TEST_CASE(test_create_item_document_missing_required_fields)
{
    web::json::value body;
    body[U("itemId")] = web::json::value::number(30);

    auto response = makePostRequest("/api/v1/item-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getCreateItemDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_item_document_duplicate)
{
    mockItemDocumentService->setCreateItemDocumentResult(std::nullopt);

    web::json::value body;
    body[U("itemId")] = web::json::value::number(10);
    body[U("documentId")] = web::json::value::number(100);

    auto response = makePostRequest("/api/v1/item-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/item-documents/{id} — Удаление связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_item_document_success)
{
    services::ItemDocumentResult deleteResult;
    deleteResult.success = true;
    mockItemDocumentService->setDeleteItemDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/item-documents/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getDeleteItemDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastDeletedItemDocumentId(), 3);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastDeleteItemDocumentUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_item_document_not_found)
{
    services::ItemDocumentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "ItemDocument not found";
    mockItemDocumentService->setDeleteItemDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/item-documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getDeleteItemDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getLastDeletedItemDocumentId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_item_document_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/item-documents/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockItemDocumentService->getDeleteItemDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
