#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_edge_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct EdgesTestFixture
{
    EdgesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockEdgeService = std::make_shared<MockEdgeService>();

        // Возвращаем числовой userId, который можно преобразовать в int64_t
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        server = std::make_unique<RestServer>("127.0.0.1", 18087);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setEdgeService(mockEdgeService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread([this]()
                                   { server->start(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~EdgesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
        web::http::http_request request(methods::POST);
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
    std::shared_ptr<MockEdgeService> mockEdgeService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(EdgesCrudTestSuite, EdgesTestFixture)

// ============================================================
// GET /api/edges — Получение списка переходов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_edges_returns_list)
{
    services::EdgesPage testPage;
    dto::Edge e1;
    e1.id = 1;
    e1.beginStateId = 10;
    e1.endStateId = 20;

    dto::Edge e2;
    e2.id = 2;
    e2.beginStateId = 20;
    e2.endStateId = 30;

    testPage.edges = { e1, e2 };
    testPage.totalCount = 2;
    mockEdgeService->setEdgesResult(testPage);

    auto response = makeGetRequest("/api/edges").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockEdgeService->getEdgesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_edges_with_filters)
{
    services::EdgesPage emptyPage;
    mockEdgeService->setEdgesResult(emptyPage);

    auto response = makeGetRequest("/api/edges?beginStateId=10&endStateId=20").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockEdgeService->getLastEdgesBeginStateId().has_value());
    BOOST_CHECK_EQUAL(*mockEdgeService->getLastEdgesBeginStateId(), 10);
    BOOST_REQUIRE(mockEdgeService->getLastEdgesEndStateId().has_value());
    BOOST_CHECK_EQUAL(*mockEdgeService->getLastEdgesEndStateId(), 20);
}

BOOST_AUTO_TEST_CASE(test_get_edges_with_pagination)
{
    services::EdgesPage emptyPage;
    mockEdgeService->setEdgesResult(emptyPage);

    auto response = makeGetRequest("/api/edges?page=2&pageSize=15").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastEdgesPage(), 2);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastEdgesPageSize(), 15);
}

// ============================================================
// GET /api/edges/{id} — Получение перехода по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_edge_by_id_success)
{
    dto::Edge edge;
    edge.id = 7;
    edge.beginStateId = 10;
    edge.endStateId = 20;
    mockEdgeService->setEdgeResult(edge);

    auto response = makeGetRequest("/api/edges/7").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastEdgeId(), 7);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 7);
    BOOST_CHECK_EQUAL(json.at(U("beginStateId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("endStateId")).as_integer(), 20);
}

BOOST_AUTO_TEST_CASE(test_get_edge_not_found)
{
    mockEdgeService->setEdgeResult(std::nullopt);

    auto response = makeGetRequest("/api/edges/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/edges — Создание перехода
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_edge_success)
{
    dto::Edge createdEdge;
    createdEdge.id = 50;
    createdEdge.beginStateId = 10;
    createdEdge.endStateId = 20;
    mockEdgeService->setCreateEdgeResult(createdEdge);

    web::json::value body;
    body[U("beginStateId")] = web::json::value::number(10);
    body[U("endStateId")] = web::json::value::number(20);

    auto response = makePostRequest("/api/edges", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockEdgeService->getCreateEdgeCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockEdgeService->getLastCreatedEdge().beginStateId, 10);
    BOOST_CHECK_EQUAL(*mockEdgeService->getLastCreatedEdge().endStateId, 20);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastCreateEdgeUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_create_edge_missing_fields)
{
    web::json::value body;
    body[U("beginStateId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/edges", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockEdgeService->getCreateEdgeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_edge_duplicate)
{
    // Создаем существующий переход
    dto::Edge existingEdge;
    existingEdge.id = 1;
    existingEdge.beginStateId = 10;
    existingEdge.endStateId = 20;

    // Настраиваем mock так, чтобы при проверке существования возвращался существующий переход
    services::EdgesPage pageWithEdge;
    pageWithEdge.edges = { existingEdge };
    pageWithEdge.totalCount = 1;
    mockEdgeService->setEdgesResult(pageWithEdge);

    // Настраиваем createEdge (хотя он не должен вызываться)
    mockEdgeService->setCreateEdgeResult(std::nullopt);

    web::json::value body;
    body[U("beginStateId")] = web::json::value::number(10);
    body[U("endStateId")] = web::json::value::number(20);

    auto response = makePostRequest("/api/edges", body).get();

    // Должен вернуть 409 Conflict, потому что переход уже существует
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
    // createEdge не должен вызываться
    BOOST_CHECK_EQUAL(mockEdgeService->getCreateEdgeCallCount(), 0);
}

// ============================================================
// GET /api/workflows/{id}/edges — Получение переходов workflow
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_workflow_edges_success)
{
    dto::Edge e1;
    e1.id = 1;
    e1.beginStateId = 10;
    e1.endStateId = 20;

    dto::Edge e2;
    e2.id = 2;
    e2.beginStateId = 20;
    e2.endStateId = 30;

    mockEdgeService->setWorkflowEdgesResult({ e1, e2 });

    auto response = makeGetRequest("/api/workflows/5/edges").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockEdgeService->getWorkflowEdgesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastWorkflowEdgesId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

// ============================================================
// DELETE /api/edges/{id} — Удаление перехода
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_edge_success)
{
    services::EdgeResult deleteResult;
    deleteResult.success = true;
    mockEdgeService->setDeleteEdgeResult(deleteResult);

    auto response = makeDeleteRequest("/api/edges/7").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockEdgeService->getDeleteEdgeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastDeletedEdgeId(), 7);
    BOOST_CHECK_EQUAL(mockEdgeService->getLastDeleteEdgeUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_edge_not_found)
{
    services::EdgeResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Edge not found";
    mockEdgeService->setDeleteEdgeResult(deleteResult);

    auto response = makeDeleteRequest("/api/edges/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_edge_requires_auth)
{
    auto response = makeDeleteRequest("/api/edges/7", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockEdgeService->getDeleteEdgeCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_edge_lifecycle)
{
    // 1. Создание
    dto::Edge newEdge;
    newEdge.id = 100;
    newEdge.beginStateId = 10;
    newEdge.endStateId = 20;
    mockEdgeService->setCreateEdgeResult(newEdge);

    web::json::value createBody;
    createBody[U("beginStateId")] = web::json::value::number(10);
    createBody[U("endStateId")] = web::json::value::number(20);

    auto createResponse = makePostRequest("/api/edges", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockEdgeService->setEdgeResult(newEdge);
    auto getResponse = makeGetRequest("/api/edges/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение переходов workflow
    mockEdgeService->setWorkflowEdgesResult({ newEdge });
    auto workflowEdgesResponse = makeGetRequest("/api/workflows/5/edges").get();
    BOOST_CHECK_EQUAL(workflowEdgesResponse.status_code(), status_codes::OK);

    // 4. Удаление
    services::EdgeResult deleteResult;
    deleteResult.success = true;
    mockEdgeService->setDeleteEdgeResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/edges/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockEdgeService->setEdgeResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/edges/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
