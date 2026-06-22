#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_board_column_service.h"
#include "tests/server_mocks/mock_board_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct BoardColumnsTestFixture
{
    BoardColumnsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockBoardService = std::make_shared<MockBoardService>();
        mockBoardColumnService = std::make_shared<MockBoardColumnService>();

        // Обычный пользователь с правами
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultBoardColumnService();

        server = std::make_unique<RestServer>("127.0.0.1", 18121);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setBoardService(mockBoardService);
        server->setBoardColumnService(mockBoardColumnService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultBoardColumnService()
    {
        // Очищаем все настройки
        mockBoardColumnService->reset();

        // Настройка списка колонок
        services::BoardColumnsPage testPage;

        dto::BoardColumn col1 = MockBoardColumnService::createTestColumn(1, 1, 1, 1, R"({"wip": 5})");
        dto::BoardColumn col2 = MockBoardColumnService::createTestColumn(2, 1, 2, 2, R"({"wip": 8})");
        dto::BoardColumn col3 = MockBoardColumnService::createTestColumn(3, 2, 1, 1, R"({"wip": 3})");

        testPage.columns = { col1, col2, col3 };
        testPage.totalCount = 3;
        mockBoardColumnService->setGetBoardColumnsResult(testPage);
        mockBoardColumnService->setGetBoardColumnResult(col1);

        // Настройка колонок по доске (только для теста get_columns_by_board)
        // НЕ ДОБАВЛЯЕМ в m_existingColumns!
        std::vector<dto::BoardColumn> boardColumns = { col1, col2 };
        mockBoardColumnService->setGetColumnsByBoardResult(boardColumns);

        // Настройка создания колонки
        dto::BoardColumn newColumn = MockBoardColumnService::createTestColumn(100, 1, 3, 3);
        mockBoardColumnService->setCreateBoardColumnResult(newColumn);

        // Настройка обновления колонки
        dto::BoardColumn updatedColumn = col1;
        updatedColumn.orderNumber = 5;
        updatedColumn.settings = R"({"wip": 10, "color": "red"})";
        mockBoardColumnService->setUpdateBoardColumnResult(updatedColumn);

        services::BoardColumnResult deleteResult;
        deleteResult.success = true;
        mockBoardColumnService->setDeleteBoardColumnResult(deleteResult);

        mockBoardColumnService->setDeleteColumnsByBoardResult(2);

        // Настройка доски для тестов
        dto::Board board = MockBoardService::createTestBoard(1, "Test Board", 10, 1);
        mockBoardService->setGetBoardResult(board);
    }

    ~BoardColumnsTestFixture()
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
    std::shared_ptr<MockBoardService> mockBoardService;
    std::shared_ptr<MockBoardColumnService> mockBoardColumnService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(BoardColumnsCrudTestSuite, BoardColumnsTestFixture)

// ============================================================
// GET /api/v1/board-columns — Получение списка колонок
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_board_columns_returns_list)
{
    auto response = makeGetRequest("/api/v1/board-columns").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getGetBoardColumnsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetBoardColumnsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_board_columns_with_pagination_params)
{
    services::BoardColumnsPage emptyPage;
    mockBoardColumnService->setGetBoardColumnsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/board-columns?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetBoardColumnsPage(), 3);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetBoardColumnsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_board_columns_filter_by_board)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockBoardColumnService->reset();

    dto::BoardColumn col = MockBoardColumnService::createTestColumn(10, 42, 1, 1);
    services::BoardColumnsPage filteredPage;
    filteredPage.columns = { col };
    filteredPage.totalCount = 1;
    mockBoardColumnService->setGetBoardColumnsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/board-columns?boardId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockBoardColumnService->getLastGetBoardColumnsBoardId().has_value());
    BOOST_CHECK_EQUAL(*mockBoardColumnService->getLastGetBoardColumnsBoardId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("boardId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_board_columns_filter_by_state)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockBoardColumnService->reset();

    dto::BoardColumn col = MockBoardColumnService::createTestColumn(20, 1, 5, 1);
    services::BoardColumnsPage filteredPage;
    filteredPage.columns = { col };
    filteredPage.totalCount = 1;
    mockBoardColumnService->setGetBoardColumnsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/board-columns?stateId=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockBoardColumnService->getLastGetBoardColumnsStateId().has_value());
    BOOST_CHECK_EQUAL(*mockBoardColumnService->getLastGetBoardColumnsStateId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("stateId")).as_integer(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_board_columns_requires_auth)
{
    auto response = makeGetRequest("/api/v1/board-columns", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getGetBoardColumnsCallCount(), 0);
}

// ============================================================
// GET /api/v1/board-columns/{id} — Получение колонки по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_board_column_by_id_success)
{
    dto::BoardColumn col = MockBoardColumnService::createTestColumn(42, 1, 2, 3);
    mockBoardColumnService->setGetBoardColumnResult(col);

    auto response = makeGetRequest("/api/v1/board-columns/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getGetBoardColumnCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetBoardColumnId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("boardId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("stateId")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("orderNumber")).as_integer(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_board_column_not_found)
{
    mockBoardColumnService->setGetBoardColumnResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/board-columns/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getGetBoardColumnCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetBoardColumnId(), 999);
}

// ============================================================
// GET /api/v1/boards/{boardId}/columns — Колонки доски
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_columns_by_board_success)
{
    auto response = makeGetRequest("/api/v1/boards/1/columns").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getGetColumnsByBoardCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastGetColumnsByBoardId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_columns_by_board_empty)
{
    mockBoardColumnService->setGetColumnsByBoardResult({});

    auto response = makeGetRequest("/api/v1/boards/999/columns").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// POST /api/v1/boards/{boardId}/columns — Создание колонки
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_board_column_success)
{
    dto::BoardColumn createdColumn = MockBoardColumnService::createTestColumn(100, 1, 3, 3);
    mockBoardColumnService->setCreateBoardColumnResult(createdColumn);

    web::json::value body;
    body[U("stateId")] = web::json::value::number(3);
    body[U("orderNumber")] = web::json::value::number(3);
    body[U("settings")] = web::json::value::string(U(R"({"wip": 5})"));

    auto response = makePostRequest("/api/v1/boards/1/columns", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getCreateBoardColumnCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastCreateBoardColumnUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("boardId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("stateId")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("orderNumber")).as_integer(), 3);
}

BOOST_AUTO_TEST_CASE(test_create_board_column_missing_required_fields)
{
    web::json::value body;
    body[U("orderNumber")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/boards/1/columns", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getCreateBoardColumnCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_board_column_missing_order_number)
{
    web::json::value body;
    body[U("stateId")] = web::json::value::number(3);

    auto response = makePostRequest("/api/v1/boards/1/columns", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getCreateBoardColumnCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_board_column_duplicate)
{
    mockBoardColumnService->setCreateBoardColumnResult(std::nullopt);

    web::json::value body;
    body[U("stateId")] = web::json::value::number(1);
    body[U("orderNumber")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/boards/1/columns", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_create_board_column_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");
    mockBoardColumnService->setCreateBoardColumnResult(std::nullopt);

    web::json::value body;
    body[U("stateId")] = web::json::value::number(3);
    body[U("orderNumber")] = web::json::value::number(3);

    auto response = makePostRequest("/api/v1/boards/1/columns", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/board-columns/{id} — Обновление колонки
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_board_column_success)
{
    dto::BoardColumn updatedColumn = MockBoardColumnService::createTestColumn(1, 1, 1, 5, R"({"wip": 10, "color": "red"})");
    mockBoardColumnService->setUpdateBoardColumnResult(updatedColumn);

    web::json::value body;
    body[U("orderNumber")] = web::json::value::number(5);
    body[U("settings")] = web::json::value::string(U(R"({"wip": 10, "color": "red"})"));

    auto response = makePutRequest("/api/v1/board-columns/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getUpdateBoardColumnCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("orderNumber")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("settings")).as_string(), U("{\"wip\": 10, \"color\": \"red\"}"));
}

BOOST_AUTO_TEST_CASE(test_update_board_column_partial)
{
    dto::BoardColumn updatedColumn = MockBoardColumnService::createTestColumn(1, 1, 1, 10);
    mockBoardColumnService->setUpdateBoardColumnResult(updatedColumn);

    web::json::value body;
    body[U("orderNumber")] = web::json::value::number(10);

    auto response = makePutRequest("/api/v1/board-columns/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getUpdateBoardColumnCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_board_column_not_found)
{
    mockBoardColumnService->setUpdateBoardColumnResult(std::nullopt);

    web::json::value body;
    body[U("orderNumber")] = web::json::value::number(10);

    auto response = makePutRequest("/api/v1/board-columns/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getUpdateBoardColumnCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/board-columns/{id} — Удаление колонки
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_board_column_success)
{
    services::BoardColumnResult deleteResult;
    deleteResult.success = true;
    mockBoardColumnService->setDeleteBoardColumnResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/board-columns/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getDeleteBoardColumnCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastDeletedBoardColumnId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_board_column_not_found)
{
    services::BoardColumnResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Board column not found";
    mockBoardColumnService->setDeleteBoardColumnResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/board-columns/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getDeleteBoardColumnCallCount(), 1);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getLastDeletedBoardColumnId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_board_column_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/board-columns/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockBoardColumnService->getDeleteBoardColumnCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_board_column_lifecycle)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockBoardColumnService->reset();

    // 1. Создание колонки
    dto::BoardColumn newColumn = MockBoardColumnService::createTestColumn(200, 1, 5, 10);
    mockBoardColumnService->setCreateBoardColumnResult(newColumn);

    web::json::value createBody;
    createBody[U("stateId")] = web::json::value::number(5);
    createBody[U("orderNumber")] = web::json::value::number(10);

    auto createResponse = makePostRequest("/api/v1/boards/1/columns", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newColumnId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newColumnId, 0);

    // 2. Чтение созданной колонки
    mockBoardColumnService->setGetBoardColumnResult(newColumn);
    auto getResponse = makeGetRequest("/api/v1/board-columns/" + std::to_string(newColumnId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление колонки
    dto::BoardColumn updatedColumn = newColumn;
    updatedColumn.orderNumber = 15;
    updatedColumn.settings = R"({"wip": 20})";
    mockBoardColumnService->setUpdateBoardColumnResult(updatedColumn);

    web::json::value updateBody;
    updateBody[U("orderNumber")] = web::json::value::number(15);
    updateBody[U("settings")] = web::json::value::string(U(R"({"wip": 20})"));

    auto updateResponse = makePutRequest("/api/v1/board-columns/" + std::to_string(newColumnId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Получение колонок доски
    std::vector<dto::BoardColumn> columns = { updatedColumn };
    mockBoardColumnService->setGetColumnsByBoardResult(columns);
    auto listResponse = makeGetRequest("/api/v1/boards/1/columns").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление колонки
    services::BoardColumnResult deleteResult;
    deleteResult.success = true;
    mockBoardColumnService->setDeleteBoardColumnResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/board-columns/" + std::to_string(newColumnId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockBoardColumnService->setGetBoardColumnResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/board-columns/" + std::to_string(newColumnId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
