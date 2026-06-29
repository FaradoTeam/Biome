#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_comment_document_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct CommentDocumentsTestFixture
{
    CommentDocumentsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockCommentDocumentService = std::make_shared<MockCommentDocumentService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultCommentDocumentService();

        server = std::make_unique<RestServer>("127.0.0.1", 18124);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setCommentDocumentService(mockCommentDocumentService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultCommentDocumentService()
    {
        // Настройка списка связей
        services::CommentDocumentsPage testPage;

        dto::CommentDocument link1;
        link1.id = 1;
        link1.commentId = 5;
        link1.documentId = 100;

        dto::CommentDocument link2;
        link2.id = 2;
        link2.commentId = 5;
        link2.documentId = 101;

        dto::CommentDocument link3;
        link3.id = 3;
        link3.commentId = 6;
        link3.documentId = 100;

        testPage.items = { link1, link2, link3 };
        testPage.totalCount = 3;
        mockCommentDocumentService->setGetCommentDocumentsResult(testPage);
        mockCommentDocumentService->setGetCommentDocumentResult(link1);

        dto::CommentDocument newLink;
        newLink.id = 100;
        newLink.commentId = 7;
        newLink.documentId = 102;
        mockCommentDocumentService->setCreateCommentDocumentResult(newLink);

        services::CommentDocumentResult deleteResult;
        deleteResult.success = true;
        mockCommentDocumentService->setDeleteCommentDocumentResult(deleteResult);

        // Документы по комментарию
        std::vector<dto::CommentDocument> commentDocs = { link1, link2 };
        mockCommentDocumentService->setGetDocumentsByCommentResult(commentDocs);

        // Комментарии по документу
        std::vector<dto::CommentDocument> docComments = { link1, link3 };
        mockCommentDocumentService->setGetCommentsByDocumentResult(docComments);
    }

    ~CommentDocumentsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18124"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18124"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18124"));
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
    std::shared_ptr<MockCommentDocumentService> mockCommentDocumentService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(CommentDocumentsCrudTestSuite, CommentDocumentsTestFixture)

// ============================================================
// GET /api/v1/comment-documents — Получение списка связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comment_documents_returns_list)
{
    auto response = makeGetRequest("/api/v1/comment-documents").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetCommentDocumentsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentDocumentsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_comment_documents_with_pagination_params)
{
    services::CommentDocumentsPage emptyPage;
    mockCommentDocumentService->setGetCommentDocumentsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/comment-documents?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentDocumentsPage(), 3);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentDocumentsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_comment_documents_filter_by_comment)
{
    services::CommentDocumentsPage filteredPage;
    dto::CommentDocument link;
    link.id = 10;
    link.commentId = 42;
    link.documentId = 100;
    filteredPage.items = { link };
    filteredPage.totalCount = 1;
    mockCommentDocumentService->setGetCommentDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/comment-documents?commentId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockCommentDocumentService->getLastGetCommentDocumentsCommentId().has_value());
    BOOST_CHECK_EQUAL(*mockCommentDocumentService->getLastGetCommentDocumentsCommentId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("commentId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_comment_documents_filter_by_document)
{
    services::CommentDocumentsPage filteredPage;
    dto::CommentDocument link;
    link.id = 20;
    link.commentId = 5;
    link.documentId = 101;
    filteredPage.items = { link };
    filteredPage.totalCount = 1;
    mockCommentDocumentService->setGetCommentDocumentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/comment-documents?documentId=101").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockCommentDocumentService->getLastGetCommentDocumentsDocumentId().has_value());
    BOOST_CHECK_EQUAL(*mockCommentDocumentService->getLastGetCommentDocumentsDocumentId(), 101);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("documentId")).as_integer(), 101);
}

BOOST_AUTO_TEST_CASE(test_get_comment_documents_requires_auth)
{
    auto response = makeGetRequest("/api/v1/comment-documents", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetCommentDocumentsCallCount(), 0);
}

// ============================================================
// GET /api/v1/comment-documents/{id} — Получение связи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comment_document_by_id_success)
{
    dto::CommentDocument link;
    link.id = 42;
    link.commentId = 5;
    link.documentId = 100;
    mockCommentDocumentService->setGetCommentDocumentResult(link);

    auto response = makeGetRequest("/api/v1/comment-documents/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetCommentDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentDocumentId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("commentId")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("documentId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_comment_document_not_found)
{
    mockCommentDocumentService->setGetCommentDocumentResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/comment-documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetCommentDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentDocumentId(), 999);
}

// ============================================================
// GET /api/v1/comments/{commentId}/documents — Документы комментария
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_documents_by_comment_success)
{
    auto response = makeGetRequest("/api/v1/comments/5/documents").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetDocumentsByCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetDocumentsByCommentId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

// ============================================================
// GET /api/v1/documents/{documentId}/comments — Комментарии документа
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comments_by_document_success)
{
    auto response = makeGetRequest("/api/v1/documents/100/comments").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getGetCommentsByDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastGetCommentsByDocumentId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

// ============================================================
// POST /api/v1/comment-documents — Создание связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_comment_document_success)
{
    dto::CommentDocument createdLink;
    createdLink.id = 100;
    createdLink.commentId = 7;
    createdLink.documentId = 102;
    mockCommentDocumentService->setCreateCommentDocumentResult(createdLink);

    web::json::value body;
    body[U("commentId")] = web::json::value::number(7);
    body[U("documentId")] = web::json::value::number(102);

    auto response = makePostRequest("/api/v1/comment-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getCreateCommentDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastCreateCommentDocumentUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("commentId")).as_integer(), 7);
    BOOST_CHECK_EQUAL(json.at(U("documentId")).as_integer(), 102);
}

BOOST_AUTO_TEST_CASE(test_create_comment_document_missing_required_fields)
{
    web::json::value body;
    body[U("commentId")] = web::json::value::number(7);

    auto response = makePostRequest("/api/v1/comment-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getCreateCommentDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_comment_document_duplicate)
{
    mockCommentDocumentService->setCreateCommentDocumentResult(std::nullopt);

    web::json::value body;
    body[U("commentId")] = web::json::value::number(5);
    body[U("documentId")] = web::json::value::number(100);

    auto response = makePostRequest("/api/v1/comment-documents", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/comment-documents/{id} — Удаление связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_comment_document_success)
{
    services::CommentDocumentResult deleteResult;
    deleteResult.success = true;
    mockCommentDocumentService->setDeleteCommentDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/comment-documents/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getDeleteCommentDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastDeletedCommentDocumentId(), 3);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastDeleteCommentDocumentUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_comment_document_not_found)
{
    services::CommentDocumentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "CommentDocument not found";
    mockCommentDocumentService->setDeleteCommentDocumentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/comment-documents/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getDeleteCommentDocumentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getLastDeletedCommentDocumentId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_comment_document_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/comment-documents/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockCommentDocumentService->getDeleteCommentDocumentCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
