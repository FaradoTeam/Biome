#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_document_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct DocumentsTestFixture
{
    DocumentsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockDocumentService = std::make_shared<MockDocumentService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultDocumentService();

        server = std::make_unique<RestServer>("127.0.0.1", 18121);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setDocumentService(mockDocumentService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultDocumentService()
    {
        // Настройка списка документов
        services::DocumentsPage testPage;

        dto::Document doc1 = MockDocumentService::createTestDocument(1, "Документ 1", "/path/doc1.pdf", "doc1.pdf", 1024, 100);
        dto::Document doc2 = MockDocumentService::createTestDocument(2, "Документ 2", "/path/doc2.docx", "doc2.docx", 2048, 100);
        dto::Document doc3 = MockDocumentService::createTestDocument(3, "Документ 3", "/path/doc3.txt", "doc3.txt", 512, 101);

        testPage.documents = { doc1, doc2, doc3 };
        testPage.totalCount = 3;
        mockDocumentService->setGetDocumentsResult(testPage);
        mockDocumentService->setGetDocumentResult(doc1);

        dto::Document newDoc = MockDocumentService::createTestDocument(100, "Новый документ", "/path/new.pdf", "new.pdf", 4096, 100);
        mockDocumentService->setCreateDocumentResult(newDoc);

        dto::Document updatedDoc = doc1;
        updatedDoc.caption = "Обновлённый документ";
        updatedDoc.description = "Новое описание";
        mockDocumentService->setUpdateDocumentResult(updatedDoc);

        services::DocumentResult deleteResult;
        deleteResult.success = true;
        mockDocumentService->setDeleteDocumentResult(deleteResult);
    }

    ~DocumentsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18121"));
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
    std::shared_ptr<MockDocumentService> mockDocumentService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(DocumentsCrudTestSuite, DocumentsTestFixture)

// ============================================================
// GET /api/v1/documents — Получение списка документов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_documents_returns_list)
{
    auto response = makeGetRequest("/api/v1/documents").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getGetDocumentsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_documents_with_pagination_params)
{
    services::DocumentsPage emptyPage;
    mockDocumentService->setGetDocumentsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/documents?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentsPage(), 3);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_documents_filter_by_user)
{
    services::DocumentsPage filteredPage;
    dto::Document doc = MockDocumentService::createTestDocument(10, "User Document", "/path/user.pdf", "user.pdf", 1024, 101);
    filteredPage.documents = { doc };
    filteredPage.totalCount = 1;
    mockDocumentService->setGetDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/documents?uploadedByUserId=101").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockDocumentService->getLastGetDocumentsUploadedByUserId().has_value());
    BOOST_CHECK_EQUAL(*mockDocumentService->getLastGetDocumentsUploadedByUserId(), 101);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("uploadedByUserId")).as_integer(), 101);
}

BOOST_AUTO_TEST_CASE(test_get_documents_search_by_caption)
{
    services::DocumentsPage filteredPage;
    dto::Document doc = MockDocumentService::createTestDocument(20, "Searchable Document", "/path/search.pdf", "search.pdf", 1024, 100);
    filteredPage.documents = { doc };
    filteredPage.totalCount = 1;
    mockDocumentService->setGetDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/documents?caption=Searchable").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentsSearchCaption(), "Searchable");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("caption")).as_string(), U("Searchable Document"));
}

BOOST_AUTO_TEST_CASE(test_get_documents_requires_auth)
{
    auto response = makeGetRequest("/api/v1/documents", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockDocumentService->getGetDocumentsCallCount(), 0);
}

// ============================================================
// GET /api/v1/documents/{id} — Получение документа по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_document_by_id_success)
{
    dto::Document doc = MockDocumentService::createTestDocument(42, "Конкретный документ", "/path/doc.pdf", "doc.pdf", 1024, 100);
    mockDocumentService->setGetDocumentResult(doc);

    auto response = makeGetRequest("/api/v1/documents/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getGetDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретный документ"));
    BOOST_CHECK_EQUAL(json.at(U("filename")).as_string(), U("doc.pdf"));
}

BOOST_AUTO_TEST_CASE(test_get_document_not_found)
{
    mockDocumentService->setGetDocumentResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockDocumentService->getGetDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastGetDocumentId(), 999);
}

// ============================================================
// POST /api/v1/documents — Создание документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_document_success)
{
    dto::Document createdDoc = MockDocumentService::createTestDocument(100, "Новый документ", "/path/new.pdf", "new.pdf", 4096, 100);
    mockDocumentService->setCreateDocumentResult(createdDoc);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый документ"));
    body[U("path")] = web::json::value::string(U("/path/new.pdf"));
    body[U("filename")] = web::json::value::string(U("new.pdf"));
    body[U("size")] = web::json::value::number(4096);
    body[U("mimeType")] = web::json::value::string(U("application/pdf"));
    body[U("description")] = web::json::value::string(U("Описание нового документа"));

    auto response = makePostRequest("/api/v1/documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockDocumentService->getCreateDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastCreateDocumentUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новый документ"));
    BOOST_CHECK_EQUAL(json.at(U("filename")).as_string(), U("new.pdf"));
}

BOOST_AUTO_TEST_CASE(test_create_document_missing_required_fields)
{
    web::json::value body;
    body[U("path")] = web::json::value::string(U("/path/doc.pdf"));
    body[U("filename")] = web::json::value::string(U("doc.pdf"));

    auto response = makePostRequest("/api/v1/documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockDocumentService->getCreateDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_document_missing_path)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Документ без пути"));
    body[U("filename")] = web::json::value::string(U("doc.pdf"));

    auto response = makePostRequest("/api/v1/documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockDocumentService->getCreateDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_document_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockDocumentService->setCreateDocumentResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка создания"));
    body[U("path")] = web::json::value::string(U("/path/doc.pdf"));
    body[U("filename")] = web::json::value::string(U("doc.pdf"));
    body[U("size")] = web::json::value::number(1024);

    auto response = makePostRequest("/api/v1/documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/documents/{id} — Обновление документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_document_success)
{
    dto::Document updatedDoc = MockDocumentService::createTestDocument(1, "Обновлённый документ", "/path/updated.pdf", "updated.pdf", 1024, 100);
    updatedDoc.description = "Новое описание";
    mockDocumentService->setUpdateDocumentResult(updatedDoc);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновлённый документ"));
    body[U("description")] = web::json::value::string(U("Новое описание"));

    auto response = makePutRequest("/api/v1/documents/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getUpdateDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastUpdateDocumentId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновлённый документ"));
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Новое описание"));
}

BOOST_AUTO_TEST_CASE(test_update_document_partial)
{
    dto::Document updatedDoc = MockDocumentService::createTestDocument(1, "Только название", "/path/doc.pdf", "doc.pdf", 1024, 100);
    mockDocumentService->setUpdateDocumentResult(updatedDoc);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Только название"));

    auto response = makePutRequest("/api/v1/documents/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockDocumentService->getUpdateDocumentCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_document_not_found)
{
    // Устанавливаем, что обновление не удалось
    mockDocumentService->setUpdateDocumentResult(std::nullopt);
    // И документ не найден при проверке
    mockDocumentService->setGetDocumentResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующий"));

    auto response = makePutRequest("/api/v1/documents/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockDocumentService->getUpdateDocumentCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_document_insufficient_permissions)
{
    // Устанавливаем, что обновление не удалось
    mockDocumentService->setUpdateDocumentResult(std::nullopt);
    // Но документ существует (возвращается при проверке)
    dto::Document existingDoc = MockDocumentService::createTestDocument(1, "Существующий документ", "/path/doc.pdf", "doc.pdf", 1024, 100);
    mockDocumentService->setGetDocumentResult(existingDoc);

    // Меняем пользователя на того, у кого нет прав
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/documents/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockDocumentService->getUpdateDocumentCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/documents/{id} — Удаление документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_document_success)
{
    services::DocumentResult deleteResult;
    deleteResult.success = true;
    mockDocumentService->setDeleteDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/documents/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockDocumentService->getDeleteDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastDeletedDocumentId(), 3);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastDeleteDocumentUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_document_not_found)
{
    services::DocumentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Document not found";
    mockDocumentService->setDeleteDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockDocumentService->getDeleteDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockDocumentService->getLastDeletedDocumentId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_document_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::DocumentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockDocumentService->setDeleteDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/documents/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_document_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/documents/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockDocumentService->getDeleteDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
