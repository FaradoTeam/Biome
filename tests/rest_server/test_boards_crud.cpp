#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_board_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct BoardsTestFixture
{
    BoardsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockBoardService = std::make_shared<MockBoardService>();

        // Обычный пользователь с правами (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultBoardService();

        server = std::make_unique<RestServer>("127.0.0.1", 18120);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setBoardService(mockBoardService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultBoardService()
    {
        // Настройка списка досок
        services::BoardsPage testPage;

        dto::Board board1 = MockBoardService::createTestBoard(1, "Канбан-доска 1", 10, 1);
        dto::Board board2 = MockBoardService::createTestBoard(2, "Канбан-доска 2", 10, 1);
        dto::Board board3 = MockBoardService::createTestBoard(3, "Канбан-доска 3", 20, 1);

        testPage.boards = { board1, board2, board3 };
        testPage.totalCount = 3;
        mockBoardService->setGetBoardsResult(testPage);
        mockBoardService->setGetBoardResult(board1);

        dto::Board newBoard = MockBoardService::createTestBoard(100, "Новая доска", 10, 1);
        mockBoardService->setCreateBoardResult(newBoard);

        dto::Board updatedBoard = board1;
        updatedBoard.caption = "Обновлённая доска";
        updatedBoard.description = "Новое описание";
        mockBoardService->setUpdateBoardResult(updatedBoard);

        services::BoardResult deleteResult;
        deleteResult.success = true;
        mockBoardService->setDeleteBoardResult(deleteResult);

        // Настройка досок по проекту
        std::vector<dto::Board> projectBoards = { board1, board2 };
        mockBoardService->setGetBoardsByProjectResult(projectBoards);

        // Настройка досок по фазе
        std::vector<dto::Board> phaseBoards = { board1 };
        mockBoardService->setGetBoardsByPhaseResult(phaseBoards);
    }

    ~BoardsTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18120"));
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
    std::shared_ptr<MockBoardService> mockBoardService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(BoardsCrudTestSuite, BoardsTestFixture)

// ============================================================
// GET /api/v1/boards — Получение списка досок
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_boards_returns_list)
{
    auto response = makeGetRequest("/api/v1/boards").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_boards_with_pagination_params)
{
    services::BoardsPage emptyPage;
    mockBoardService->setGetBoardsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/boards?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardsPage(), 3);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_boards_filter_by_project)
{
    services::BoardsPage filteredPage;
    dto::Board board = MockBoardService::createTestBoard(10, "Filtered Board", 42, 1);
    filteredPage.boards = { board };
    filteredPage.totalCount = 1;
    mockBoardService->setGetBoardsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/boards?projectId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockBoardService->getLastGetBoardsProjectId().has_value());
    BOOST_CHECK_EQUAL(*mockBoardService->getLastGetBoardsProjectId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("projectId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_boards_filter_by_phase)
{
    services::BoardsPage filteredPage;
    dto::Board board = MockBoardService::createTestBoard(20, "Phase Board", 10, 1, 5);
    filteredPage.boards = { board };
    filteredPage.totalCount = 1;
    mockBoardService->setGetBoardsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/boards?phaseId=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockBoardService->getLastGetBoardsPhaseId().has_value());
    BOOST_CHECK_EQUAL(*mockBoardService->getLastGetBoardsPhaseId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("phaseId")).as_integer(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_boards_requires_auth)
{
    auto response = makeGetRequest("/api/v1/boards", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardsCallCount(), 0);
}

// ============================================================
// GET /api/v1/boards/{id} — Получение доски по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_board_by_id_success)
{
    dto::Board board = MockBoardService::createTestBoard(42, "Конкретная доска", 10, 1);
    mockBoardService->setGetBoardResult(board);

    auto response = makeGetRequest("/api/v1/boards/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретная доска"));
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_board_not_found)
{
    mockBoardService->setGetBoardResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/boards/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_board_invalid_id)
{
    auto response = makeGetRequest("/api/v1/boards/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// GET /api/v1/projects/{projectId}/boards — Доски проекта
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_boards_by_project_success)
{
    auto response = makeGetRequest("/api/v1/projects/10/boards").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardsByProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardsByProjectId(), 10);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_boards_by_project_not_found)
{
    mockBoardService->setGetBoardsByProjectResult({});

    auto response = makeGetRequest("/api/v1/projects/999/boards").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// GET /api/v1/phases/{phaseId}/boards — Доски фазы
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_boards_by_phase_success)
{
    auto response = makeGetRequest("/api/v1/phases/5/boards").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getGetBoardsByPhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastGetBoardsByPhaseId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 1);
}

// ============================================================
// POST /api/v1/boards — Создание доски
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_board_success)
{
    dto::Board createdBoard = MockBoardService::createTestBoard(100, "Новая доска", 10, 1);
    mockBoardService->setCreateBoardResult(createdBoard);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новая доска"));
    body[U("projectId")] = web::json::value::number(10);
    body[U("workflowId")] = web::json::value::number(1);
    body[U("description")] = web::json::value::string(U("Описание новой доски"));

    auto response = makePostRequest("/api/v1/boards", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockBoardService->getCreateBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastCreateBoardUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новая доска"));
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 10);
}

BOOST_AUTO_TEST_CASE(test_create_board_missing_required_fields)
{
    web::json::value body;
    body[U("projectId")] = web::json::value::number(10);
    body[U("workflowId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/boards", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockBoardService->getCreateBoardCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_board_missing_project_id)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Доска без проекта"));
    body[U("workflowId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/boards", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockBoardService->getCreateBoardCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_board_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockBoardService->setCreateBoardResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка создания"));
    body[U("projectId")] = web::json::value::number(10);
    body[U("workflowId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/boards", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/boards/{id} — Обновление доски
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_board_success)
{
    dto::Board updatedBoard = MockBoardService::createTestBoard(1, "Обновлённая доска", 10, 1);
    updatedBoard.description = "Новое описание";
    mockBoardService->setUpdateBoardResult(updatedBoard);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновлённая доска"));
    body[U("description")] = web::json::value::string(U("Новое описание"));

    auto response = makePutRequest("/api/v1/boards/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getUpdateBoardCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновлённая доска"));
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Новое описание"));
}

BOOST_AUTO_TEST_CASE(test_update_board_partial)
{
    dto::Board updatedBoard = MockBoardService::createTestBoard(1, "Только название", 10, 1);
    mockBoardService->setUpdateBoardResult(updatedBoard);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Только название"));

    auto response = makePutRequest("/api/v1/boards/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardService->getUpdateBoardCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_board_not_found)
{
    mockBoardService->setUpdateBoardResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующая"));

    auto response = makePutRequest("/api/v1/boards/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardService->getUpdateBoardCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/boards/{id} — Удаление доски
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_board_success)
{
    services::BoardResult deleteResult;
    deleteResult.success = true;
    mockBoardService->setDeleteBoardResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/boards/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockBoardService->getDeleteBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastDeletedBoardId(), 3);
    BOOST_CHECK_EQUAL(mockBoardService->getLastDeleteBoardUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_board_not_found)
{
    services::BoardResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Board not found";
    mockBoardService->setDeleteBoardResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/boards/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardService->getDeleteBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardService->getLastDeletedBoardId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_board_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    services::BoardResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockBoardService->setDeleteBoardResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/boards/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_board_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/boards/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockBoardService->getDeleteBoardCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_board_lifecycle)
{
    // 1. Создание доски
    dto::Board newBoard = MockBoardService::createTestBoard(200, "Жизненный цикл доски", 10, 1);
    mockBoardService->setCreateBoardResult(newBoard);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл доски"));
    createBody[U("projectId")] = web::json::value::number(10);
    createBody[U("workflowId")] = web::json::value::number(1);

    auto createResponse = makePostRequest("/api/v1/boards", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newBoardId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newBoardId, 0);

    // 2. Чтение созданной доски
    mockBoardService->setGetBoardResult(newBoard);
    auto getResponse = makeGetRequest("/api/v1/boards/" + std::to_string(newBoardId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление доски
    dto::Board updatedBoard = newBoard;
    updatedBoard.caption = "Обновлённый жизненный цикл";
    mockBoardService->setUpdateBoardResult(updatedBoard);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновлённый жизненный цикл"));

    auto updateResponse = makePutRequest("/api/v1/boards/" + std::to_string(newBoardId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Получение списка досок
    services::BoardsPage listPage;
    listPage.boards = { updatedBoard };
    listPage.totalCount = 1;
    mockBoardService->setGetBoardsResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/boards").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление доски
    services::BoardResult deleteResult;
    deleteResult.success = true;
    mockBoardService->setDeleteBoardResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/boards/" + std::to_string(newBoardId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockBoardService->setGetBoardResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/boards/" + std::to_string(newBoardId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
