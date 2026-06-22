#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_workflow_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct WorkflowsTestFixture
{
    WorkflowsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockWorkflowService = std::make_shared<MockWorkflowService>();

        // Супер-админ (userId=1) для создания/обновления/удаления рабочих процессов
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18085);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setWorkflowService(mockWorkflowService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~WorkflowsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
    std::shared_ptr<MockWorkflowService> mockWorkflowService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(WorkflowsCrudTestSuite, WorkflowsTestFixture)

// ============================================================
// GET /api/v1/workflows — Получение списка рабочих процессов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_workflows_returns_list)
{
    services::WorkflowsPage testPage;
    dto::Workflow wf1;
    wf1.id = 1;
    wf1.caption = "Scrum";
    wf1.description = "Scrum workflow";

    dto::Workflow wf2;
    wf2.id = 2;
    wf2.caption = "Kanban";
    wf2.description = "Kanban workflow";

    testPage.workflows = { wf1, wf2 };
    testPage.totalCount = 2;
    mockWorkflowService->setWorkflowsResult(testPage);

    auto response = makeGetRequest("/api/v1/workflows").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockWorkflowService->getWorkflowsCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_workflows_with_pagination)
{
    services::WorkflowsPage emptyPage;
    mockWorkflowService->setWorkflowsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/workflows?page=2&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastWorkflowsPage(), 2);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastWorkflowsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_workflows_empty_list)
{
    services::WorkflowsPage emptyPage;
    mockWorkflowService->setWorkflowsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/workflows").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_workflows_requires_auth)
{
    auto response = makeGetRequest("/api/v1/workflows", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockWorkflowService->getWorkflowsCallCount(), 0);
}

// ============================================================
// GET /api/v1/workflows/{id} — Получение рабочего процесса по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_workflow_by_id_success)
{
    dto::Workflow workflow;
    workflow.id = 42;
    workflow.caption = "Specific Workflow";
    workflow.description = "Test description";
    mockWorkflowService->setWorkflowResult(workflow);

    auto response = makeGetRequest("/api/v1/workflows/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockWorkflowService->getWorkflowCallCount(), 1);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastWorkflowId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Specific Workflow"));
}

BOOST_AUTO_TEST_CASE(test_get_workflow_not_found)
{
    mockWorkflowService->setWorkflowResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/workflows/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockWorkflowService->getWorkflowCallCount(), 1);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastWorkflowId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_workflow_invalid_id)
{
    auto response = makeGetRequest("/api/v1/workflows/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/workflows — Создание рабочего процесса
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_workflow_success)
{
    dto::Workflow createdWorkflow;
    createdWorkflow.id = 100;
    createdWorkflow.caption = "New Workflow";
    createdWorkflow.description = "New description";
    mockWorkflowService->setCreateWorkflowResult(createdWorkflow);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("New Workflow"));
    body[U("description")] = web::json::value::string(U("New description"));

    auto response = makePostRequest("/api/v1/workflows", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockWorkflowService->getCreateWorkflowCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockWorkflowService->getLastCreatedWorkflow().caption, "New Workflow");
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastCreateWorkflowUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("New Workflow"));
}

BOOST_AUTO_TEST_CASE(test_create_workflow_missing_caption)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("No caption"));

    auto response = makePostRequest("/api/v1/workflows", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockWorkflowService->getCreateWorkflowCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_workflow_duplicate)
{
    // Создаем существующий workflow
    dto::Workflow existingWorkflow;
    existingWorkflow.id = 1;
    existingWorkflow.caption = "Existing Workflow";

    // Настраиваем mock с существующим workflow
    mockWorkflowService->addWorkflow(existingWorkflow);

    // Настраиваем createWorkflow (хотя он не должен вызываться)
    mockWorkflowService->setCreateWorkflowResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Existing Workflow"));

    auto response = makePostRequest("/api/v1/workflows", body).get();

    // Должен вернуть 409 Conflict, потому что workflow с таким названием уже существует
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
    // createWorkflow не должен вызываться
    BOOST_CHECK_EQUAL(mockWorkflowService->getCreateWorkflowCallCount(), 0);
}

// ============================================================
// PUT /api/v1/workflows/{id} — Обновление рабочего процесса
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_workflow_success)
{
    dto::Workflow updatedWorkflow;
    updatedWorkflow.id = 1;
    updatedWorkflow.caption = "Updated Workflow";
    updatedWorkflow.description = "Updated description";
    mockWorkflowService->setUpdateWorkflowResult(updatedWorkflow);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Updated Workflow"));

    auto response = makePutRequest("/api/v1/workflows/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockWorkflowService->getUpdateWorkflowCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockWorkflowService->getLastUpdatedWorkflow().id, 1);
    BOOST_CHECK_EQUAL(*mockWorkflowService->getLastUpdatedWorkflow().caption, "Updated Workflow");
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastUpdateWorkflowUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_workflow_not_found)
{
    mockWorkflowService->setUpdateWorkflowResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Ghost"));

    auto response = makePutRequest("/api/v1/workflows/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockWorkflowService->getUpdateWorkflowCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/workflows/{id} — Удаление рабочего процесса
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_workflow_success)
{
    services::WorkflowResult deleteResult;
    deleteResult.success = true;
    mockWorkflowService->setDeleteWorkflowResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/workflows/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockWorkflowService->getDeleteWorkflowCallCount(), 1);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastDeletedWorkflowId(), 1);
    BOOST_CHECK_EQUAL(mockWorkflowService->getLastDeleteWorkflowUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_workflow_not_found)
{
    services::WorkflowResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Workflow not found";
    mockWorkflowService->setDeleteWorkflowResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/workflows/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockWorkflowService->getDeleteWorkflowCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_workflow_with_dependencies)
{
    services::WorkflowResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 409;
    deleteResult.errorMessage = "Cannot delete: workflow has states";
    mockWorkflowService->setDeleteWorkflowResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/workflows/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
    BOOST_CHECK_EQUAL(mockWorkflowService->getDeleteWorkflowCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_workflow_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/workflows/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockWorkflowService->getDeleteWorkflowCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_workflow_lifecycle)
{
    // 1. Создание
    dto::Workflow newWorkflow;
    newWorkflow.id = 100;
    newWorkflow.caption = "Lifecycle Workflow";
    newWorkflow.description = "Testing lifecycle";
    mockWorkflowService->setCreateWorkflowResult(newWorkflow);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Lifecycle Workflow"));

    auto createResponse = makePostRequest("/api/v1/workflows", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockWorkflowService->setWorkflowResult(newWorkflow);
    auto getResponse = makeGetRequest("/api/v1/workflows/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::Workflow updatedWorkflow = newWorkflow;
    updatedWorkflow.description = "Updated lifecycle description";
    mockWorkflowService->setUpdateWorkflowResult(updatedWorkflow);

    web::json::value updateBody;
    updateBody[U("description")] = web::json::value::string(U("Updated lifecycle description"));

    auto updateResponse = makePutRequest("/api/v1/workflows/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    services::WorkflowResult deleteResult;
    deleteResult.success = true;
    mockWorkflowService->setDeleteWorkflowResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/workflows/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockWorkflowService->setWorkflowResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/workflows/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
