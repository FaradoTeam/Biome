#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_comment_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct CommentsTestFixture
{
    CommentsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockCommentService = std::make_shared<MockCommentService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultCommentService();

        server = std::make_unique<RestServer>("127.0.0.1", 18122);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setCommentService(mockCommentService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultCommentService()
    {
        // Настройка списка комментариев
        services::CommentsPage testPage;

        auto now = std::chrono::system_clock::now();
        
        dto::Comment comment1;
        comment1.id = 1;
        comment1.userId = 100;
        comment1.itemId = 10;
        comment1.content = "Первый комментарий";
        comment1.createdAt = now;

        dto::Comment comment2;
        comment2.id = 2;
        comment2.userId = 100;
        comment2.itemId = 10;
        comment2.content = "Второй комментарий";
        comment2.createdAt = now + std::chrono::seconds(60);

        dto::Comment comment3;
        comment3.id = 3;
        comment3.userId = 101;
        comment3.itemId = 20;
        comment3.content = "Комментарий от другого пользователя";
        comment3.createdAt = now + std::chrono::seconds(120);

        testPage.comments = { comment1, comment2, comment3 };
        testPage.totalCount = 3;
        mockCommentService->setGetCommentsResult(testPage);
        mockCommentService->setGetCommentResult(comment1);

        dto::Comment newComment;
        newComment.id = 100;
        newComment.userId = 100;
        newComment.itemId = 10;
        newComment.content = "Новый комментарий";
        newComment.createdAt = now;
        mockCommentService->setCreateCommentResult(newComment);

        dto::Comment updatedComment = comment1;
        updatedComment.content = "Обновлённый комментарий";
        mockCommentService->setUpdateCommentResult(updatedComment);

        services::CommentResult deleteResult;
        deleteResult.success = true;
        mockCommentService->setDeleteCommentResult(deleteResult);

        // Комментарии по элементу
        std::vector<dto::Comment> itemComments = { comment1, comment2 };
        mockCommentService->setGetCommentsByItemResult(itemComments);
    }

    ~CommentsTestFixture()
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
    std::shared_ptr<MockCommentService> mockCommentService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(CommentsCrudTestSuite, CommentsTestFixture)

// ============================================================
// GET /api/v1/comments — Получение списка комментариев
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comments_returns_list)
{
    auto response = makeGetRequest("/api/v1/comments").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentService->getGetCommentsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_comments_with_pagination_params)
{
    services::CommentsPage emptyPage;
    mockCommentService->setGetCommentsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/comments?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentsPage(), 3);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_comments_filter_by_item)
{
    services::CommentsPage filteredPage;
    auto now = std::chrono::system_clock::now();
    dto::Comment comment;
    comment.id = 10;
    comment.userId = 100;
    comment.itemId = 42;
    comment.content = "Комментарий к элементу 42";
    comment.createdAt = now;
    filteredPage.comments = { comment };
    filteredPage.totalCount = 1;
    mockCommentService->setGetCommentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/comments?itemId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockCommentService->getLastGetCommentsItemId().has_value());
    BOOST_CHECK_EQUAL(*mockCommentService->getLastGetCommentsItemId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_comments_filter_by_user)
{
    services::CommentsPage filteredPage;
    auto now = std::chrono::system_clock::now();
    dto::Comment comment;
    comment.id = 20;
    comment.userId = 101;
    comment.itemId = 10;
    comment.content = "Комментарий от пользователя 101";
    comment.createdAt = now;
    filteredPage.comments = { comment };
    filteredPage.totalCount = 1;
    mockCommentService->setGetCommentsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/comments?userId=101").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockCommentService->getLastGetCommentsFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockCommentService->getLastGetCommentsFilterUserId(), 101);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 101);
}

BOOST_AUTO_TEST_CASE(test_get_comments_requires_auth)
{
    auto response = makeGetRequest("/api/v1/comments", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockCommentService->getGetCommentsCallCount(), 0);
}

// ============================================================
// GET /api/v1/comments/{id} — Получение комментария по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comment_by_id_success)
{
    auto now = std::chrono::system_clock::now();
    dto::Comment comment;
    comment.id = 42;
    comment.userId = 100;
    comment.itemId = 10;
    comment.content = "Конкретный комментарий";
    comment.createdAt = now;
    mockCommentService->setGetCommentResult(comment);

    auto response = makeGetRequest("/api/v1/comments/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentService->getGetCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Конкретный комментарий"));
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_comment_not_found)
{
    mockCommentService->setGetCommentResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/comments/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockCommentService->getGetCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentId(), 999);
}

// ============================================================
// GET /api/v1/items/{itemId}/comments — Комментарии элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_comments_by_item_success)
{
    auto response = makeGetRequest("/api/v1/items/10/comments").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentService->getGetCommentsByItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastGetCommentsByItemId(), 10);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_comments_by_item_not_found)
{
    mockCommentService->setGetCommentsByItemResult({});

    auto response = makeGetRequest("/api/v1/items/999/comments").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// POST /api/v1/comments — Создание комментария
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_comment_success)
{
    auto now = std::chrono::system_clock::now();
    dto::Comment createdComment;
    createdComment.id = 100;
    createdComment.userId = 100;
    createdComment.itemId = 10;
    createdComment.content = "Новый комментарий";
    createdComment.createdAt = now;
    mockCommentService->setCreateCommentResult(createdComment);

    web::json::value body;
    body[U("content")] = web::json::value::string(U("Новый комментарий"));
    body[U("itemId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/comments", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockCommentService->getCreateCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastCreateCommentUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Новый комментарий"));
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_create_comment_missing_required_fields)
{
    web::json::value body;
    body[U("itemId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/comments", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockCommentService->getCreateCommentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_comment_missing_item_id)
{
    web::json::value body;
    body[U("content")] = web::json::value::string(U("Комментарий без элемента"));

    auto response = makePostRequest("/api/v1/comments", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockCommentService->getCreateCommentCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_comment_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockCommentService->setCreateCommentResult(std::nullopt);

    web::json::value body;
    body[U("content")] = web::json::value::string(U("Попытка создания"));
    body[U("itemId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/comments", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/comments/{id} — Обновление комментария
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_comment_success)
{
    auto now = std::chrono::system_clock::now();
    dto::Comment updatedComment;
    updatedComment.id = 1;
    updatedComment.userId = 100;
    updatedComment.itemId = 10;
    updatedComment.content = "Обновлённый комментарий";
    updatedComment.createdAt = now;
    mockCommentService->setUpdateCommentResult(updatedComment);

    web::json::value body;
    body[U("content")] = web::json::value::string(U("Обновлённый комментарий"));

    auto response = makePutRequest("/api/v1/comments/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockCommentService->getUpdateCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastUpdateCommentId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Обновлённый комментарий"));
}

BOOST_AUTO_TEST_CASE(test_update_comment_not_found)
{
    // Устанавливаем, что обновление не удалось
    mockCommentService->setUpdateCommentResult(std::nullopt);
    // И комментарий не найден при проверке
    mockCommentService->setGetCommentResult(std::nullopt);

    web::json::value body;
    body[U("content")] = web::json::value::string(U("Несуществующий"));

    auto response = makePutRequest("/api/v1/comments/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockCommentService->getUpdateCommentCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_comment_insufficient_permissions)
{
    // Устанавливаем, что обновление не удалось
    mockCommentService->setUpdateCommentResult(std::nullopt);
    // Но комментарий существует (возвращается при проверке)
    auto now = std::chrono::system_clock::now();
    dto::Comment existingComment;
    existingComment.id = 1;
    existingComment.userId = 100;
    existingComment.itemId = 10;
    existingComment.content = "Существующий комментарий";
    existingComment.createdAt = now;
    mockCommentService->setGetCommentResult(existingComment);

    // Меняем пользователя на того, у кого нет прав
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    web::json::value body;
    body[U("content")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/comments/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockCommentService->getUpdateCommentCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/comments/{id} — Удаление комментария
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_comment_success)
{
    services::CommentResult deleteResult;
    deleteResult.success = true;
    mockCommentService->setDeleteCommentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/comments/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockCommentService->getDeleteCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastDeletedCommentId(), 3);
    BOOST_CHECK_EQUAL(mockCommentService->getLastDeleteCommentUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_comment_not_found)
{
    services::CommentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Comment not found";
    mockCommentService->setDeleteCommentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/comments/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockCommentService->getDeleteCommentCallCount(), 1);
    BOOST_CHECK_EQUAL(mockCommentService->getLastDeletedCommentId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_comment_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::CommentResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockCommentService->setDeleteCommentResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/comments/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_comment_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/comments/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockCommentService->getDeleteCommentCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
