#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_team_message_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct TeamMessagesTestFixture
{
    TeamMessagesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockTeamMessageService = std::make_shared<MockTeamMessageService>();

        // Обычный пользователь с правами
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultTeamMessageService();

        server = std::make_unique<RestServer>("127.0.0.1", 18123);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setTeamMessageService(mockTeamMessageService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultTeamMessageService()
    {
        // Настройка списка сообщений
        services::TeamMessagesPage testPage;

        auto now = std::chrono::system_clock::now();
        dto::TeamMessage msg1 = MockTeamMessageService::createTestMessage(
            1, 100, 10, "Сообщение в команде 1", now
        );
        dto::TeamMessage msg2 = MockTeamMessageService::createTestMessage(
            2, 200, 10, "Сообщение в команде 2", now + std::chrono::minutes(1)
        );
        dto::TeamMessage msg3 = MockTeamMessageService::createTestMessage(
            3, 100, 20, "Сообщение в другой команде", now + std::chrono::minutes(2)
        );

        testPage.messages = { msg1, msg2, msg3 };
        testPage.totalCount = 3;
        mockTeamMessageService->setGetMessagesResult(testPage);
        mockTeamMessageService->setGetMessageResult(msg1);

        // Настройка отправки сообщения
        dto::TeamMessage newMsg = MockTeamMessageService::createTestMessage(
            100, 100, 10, "Новое сообщение в команде", now
        );
        mockTeamMessageService->setSendMessageResult(newMsg);

        // Настройка сообщений команды
        std::vector<dto::TeamMessage> teamMessages = { msg1, msg2 };
        mockTeamMessageService->setGetTeamMessagesResult(teamMessages);

        // Настройка удаления
        services::TeamMessageResult deleteResult;
        deleteResult.success = true;
        mockTeamMessageService->setDeleteMessageResult(deleteResult);
    }

    ~TeamMessagesTestFixture()
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
    std::shared_ptr<MockTeamMessageService> mockTeamMessageService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(TeamMessagesCrudTestSuite, TeamMessagesTestFixture)

// ============================================================
// GET /api/v1/team-messages — Получение списка сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_team_messages_returns_list)
{
    auto response = makeGetRequest("/api/v1/team-messages").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getGetMessagesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetMessagesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_team_messages_with_pagination_params)
{
    services::TeamMessagesPage emptyPage;
    mockTeamMessageService->setGetMessagesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/team-messages?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetMessagesPage(), 3);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetMessagesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_team_messages_filter_by_team)
{
    mockTeamMessageService->reset();

    auto now = std::chrono::system_clock::now();
    dto::TeamMessage msg = MockTeamMessageService::createTestMessage(
        10, 100, 10, "Фильтрованное сообщение", now
    );
    services::TeamMessagesPage filteredPage;
    filteredPage.messages = { msg };
    filteredPage.totalCount = 1;
    mockTeamMessageService->setGetMessagesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/team-messages?teamId=10").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockTeamMessageService->getLastGetMessagesTeamId().has_value());
    BOOST_CHECK_EQUAL(*mockTeamMessageService->getLastGetMessagesTeamId(), 10);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("teamId")).as_integer(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_team_messages_filter_by_sender)
{
    mockTeamMessageService->reset();

    auto now = std::chrono::system_clock::now();
    dto::TeamMessage msg = MockTeamMessageService::createTestMessage(
        20, 200, 10, "От пользователя 200", now
    );
    services::TeamMessagesPage filteredPage;
    filteredPage.messages = { msg };
    filteredPage.totalCount = 1;
    mockTeamMessageService->setGetMessagesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/team-messages?senderUserId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockTeamMessageService->getLastGetMessagesSenderUserId().has_value());
    BOOST_CHECK_EQUAL(*mockTeamMessageService->getLastGetMessagesSenderUserId(), 200);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("senderUserId")).as_integer(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_team_messages_requires_auth)
{
    auto response = makeGetRequest("/api/v1/team-messages", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getGetMessagesCallCount(), 0);
}

// ============================================================
// GET /api/v1/team-messages/{id} — Получение сообщения по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_team_message_by_id_success)
{
    auto now = std::chrono::system_clock::now();
    dto::TeamMessage msg = MockTeamMessageService::createTestMessage(
        42, 100, 10, "Конкретное сообщение", now
    );
    mockTeamMessageService->setGetMessageResult(msg);

    auto response = makeGetRequest("/api/v1/team-messages/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getGetMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetMessageId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("senderUserId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("teamId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Конкретное сообщение"));
}

BOOST_AUTO_TEST_CASE(test_get_team_message_not_found)
{
    mockTeamMessageService->setGetMessageResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/team-messages/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getGetMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetMessageId(), 999);
}

// ============================================================
// GET /api/v1/teams/{teamId}/messages — Сообщения команды
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_team_messages_by_team_success)
{
    auto response = makeGetRequest("/api/v1/teams/10/messages").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getGetTeamMessagesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetTeamMessagesTeamId(), 10);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastGetTeamMessagesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_team_messages_by_team_empty)
{
    mockTeamMessageService->setGetTeamMessagesResult({});

    auto response = makeGetRequest("/api/v1/teams/999/messages").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// POST /api/v1/team-messages — Отправка сообщения в команду
// ============================================================

BOOST_AUTO_TEST_CASE(test_send_team_message_success)
{
    auto now = std::chrono::system_clock::now();
    dto::TeamMessage sentMsg = MockTeamMessageService::createTestMessage(
        100, 100, 10, "Привет команде!", now
    );
    mockTeamMessageService->setSendMessageResult(sentMsg);

    web::json::value body;
    body[U("teamId")] = web::json::value::number(10);
    body[U("content")] = web::json::value::string(U("Привет команде!"));

    auto response = makePostRequest("/api/v1/team-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getSendMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastSendMessageSenderUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("senderUserId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("teamId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Привет команде!"));
}

BOOST_AUTO_TEST_CASE(test_send_team_message_missing_team_id)
{
    web::json::value body;
    body[U("content")] = web::json::value::string(U("Сообщение без команды"));

    auto response = makePostRequest("/api/v1/team-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getSendMessageCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_send_team_message_missing_content)
{
    web::json::value body;
    body[U("teamId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/team-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getSendMessageCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_send_team_message_not_team_member)
{
    mockTeamMessageService->setSendMessageResult(std::nullopt);

    web::json::value body;
    body[U("teamId")] = web::json::value::number(999);
    body[U("content")] = web::json::value::string(U("Не член команды"));

    auto response = makePostRequest("/api/v1/team-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_send_team_message_requires_auth)
{
    web::json::value body;
    body[U("teamId")] = web::json::value::number(10);
    body[U("content")] = web::json::value::string(U("Без авторизации"));

    auto response = makePostRequest("/api/v1/team-messages", body, "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getSendMessageCallCount(), 0);
}

// ============================================================
// DELETE /api/v1/team-messages/{id} — Удаление сообщения
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_team_message_success)
{
    services::TeamMessageResult deleteResult;
    deleteResult.success = true;
    mockTeamMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/team-messages/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getDeleteMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastDeletedMessageId(), 3);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastDeleteMessageUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_team_message_not_found)
{
    services::TeamMessageResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Message not found";
    mockTeamMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/team-messages/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getDeleteMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getLastDeletedMessageId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_team_message_not_sender)
{
    services::TeamMessageResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Only sender can delete";
    mockTeamMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/team-messages/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_team_message_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/team-messages/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockTeamMessageService->getDeleteMessageCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_team_message_lifecycle)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockTeamMessageService->reset();

    auto now = std::chrono::system_clock::now();

    // 1. Отправка сообщения в команду
    dto::TeamMessage sentMsg = MockTeamMessageService::createTestMessage(
        200, 100, 10, "Привет команде!", now
    );
    mockTeamMessageService->setSendMessageResult(sentMsg);

    web::json::value sendBody;
    sendBody[U("teamId")] = web::json::value::number(10);
    sendBody[U("content")] = web::json::value::string(U("Привет команде!"));

    auto sendResponse = makePostRequest("/api/v1/team-messages", sendBody).get();
    BOOST_CHECK_EQUAL(sendResponse.status_code(), status_codes::Created);
    auto sendJson = sendResponse.extract_json().get();
    int64_t newMsgId = sendJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newMsgId, 0);

    // 2. Чтение сообщения
    mockTeamMessageService->setGetMessageResult(sentMsg);
    auto getResponse = makeGetRequest("/api/v1/team-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение всех сообщений команды
    std::vector<dto::TeamMessage> teamMessages = { sentMsg };
    mockTeamMessageService->setGetTeamMessagesResult(teamMessages);
    auto teamResponse = makeGetRequest("/api/v1/teams/10/messages").get();
    BOOST_CHECK_EQUAL(teamResponse.status_code(), status_codes::OK);

    // 4. Получение списка сообщений
    services::TeamMessagesPage listPage;
    listPage.messages = { sentMsg };
    listPage.totalCount = 1;
    mockTeamMessageService->setGetMessagesResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/team-messages").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление сообщения
    services::TeamMessageResult deleteResult;
    deleteResult.success = true;
    mockTeamMessageService->setDeleteMessageResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/team-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockTeamMessageService->setGetMessageResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/team-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
