#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_private_message_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct PrivateMessagesTestFixture
{
    PrivateMessagesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockPrivateMessageService = std::make_shared<MockPrivateMessageService>();

        // Обычный пользователь с правами
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultPrivateMessageService();

        server = std::make_unique<RestServer>("127.0.0.1", 18122);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setPrivateMessageService(mockPrivateMessageService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultPrivateMessageService()
    {
        // Настройка списка сообщений
        services::PrivateMessagesPage testPage;

        auto now = std::chrono::system_clock::now();
        dto::PrivateMessage msg1 = MockPrivateMessageService::createTestMessage(
            1, 100, 200, "Привет!", false, now
        );
        dto::PrivateMessage msg2 = MockPrivateMessageService::createTestMessage(
            2, 200, 100, "Здравствуй!", false, now + std::chrono::minutes(1)
        );
        dto::PrivateMessage msg3 = MockPrivateMessageService::createTestMessage(
            3, 100, 300, "Сообщение для другого", true, now + std::chrono::minutes(2)
        );

        testPage.messages = { msg1, msg2, msg3 };
        testPage.totalCount = 3;
        mockPrivateMessageService->setGetMessagesResult(testPage);
        mockPrivateMessageService->setGetMessageResult(msg1);

        // Настройка отправки сообщения
        dto::PrivateMessage newMsg = MockPrivateMessageService::createTestMessage(
            100, 100, 200, "Новое сообщение", false, now
        );
        mockPrivateMessageService->setSendMessageResult(newMsg);

        // Настройка переписки
        std::vector<dto::PrivateMessage> conversation = { msg1, msg2 };
        mockPrivateMessageService->setGetConversationResult(conversation);

        // Настройка отметки о прочтении
        services::PrivateMessageResult viewResult;
        viewResult.success = true;
        mockPrivateMessageService->setMarkAsViewedResult(viewResult);

        // Настройка удаления
        services::PrivateMessageResult deleteResult;
        deleteResult.success = true;
        mockPrivateMessageService->setDeleteMessageResult(deleteResult);

        // Настройка подсчёта непрочитанных
        mockPrivateMessageService->setCountUnviewedResult(1);
    }

    ~PrivateMessagesTestFixture()
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
    std::shared_ptr<MockPrivateMessageService> mockPrivateMessageService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(PrivateMessagesCrudTestSuite, PrivateMessagesTestFixture)

// ============================================================
// GET /api/v1/private-messages — Получение списка сообщений
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_private_messages_returns_list)
{
    auto response = makeGetRequest("/api/v1/private-messages").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getGetMessagesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetMessagesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_private_messages_with_pagination_params)
{
    services::PrivateMessagesPage emptyPage;
    mockPrivateMessageService->setGetMessagesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/private-messages?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetMessagesPage(), 3);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetMessagesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_private_messages_filter_by_user)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockPrivateMessageService->reset();

    auto now = std::chrono::system_clock::now();
    dto::PrivateMessage msg = MockPrivateMessageService::createTestMessage(
        10, 100, 200, "Фильтрованное сообщение", false, now
    );
    services::PrivateMessagesPage filteredPage;
    filteredPage.messages = { msg };
    filteredPage.totalCount = 1;
    mockPrivateMessageService->setGetMessagesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/private-messages?withUserId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPrivateMessageService->getLastGetMessagesFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockPrivateMessageService->getLastGetMessagesFilterUserId(), 200);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_private_messages_filter_by_viewed)
{
    mockPrivateMessageService->reset();

    auto now = std::chrono::system_clock::now();
    dto::PrivateMessage msg = MockPrivateMessageService::createTestMessage(
        20, 100, 200, "Прочитанное", true, now
    );
    services::PrivateMessagesPage filteredPage;
    filteredPage.messages = { msg };
    filteredPage.totalCount = 1;
    mockPrivateMessageService->setGetMessagesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/private-messages?isViewed=true").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPrivateMessageService->getLastGetMessagesIsViewed().has_value());
    BOOST_CHECK_EQUAL(*mockPrivateMessageService->getLastGetMessagesIsViewed(), true);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("isViewed")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_private_messages_requires_auth)
{
    auto response = makeGetRequest("/api/v1/private-messages", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getGetMessagesCallCount(), 0);
}

// ============================================================
// GET /api/v1/private-messages/{id} — Получение сообщения по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_private_message_by_id_success)
{
    auto now = std::chrono::system_clock::now();
    dto::PrivateMessage msg = MockPrivateMessageService::createTestMessage(
        42, 100, 200, "Конкретное сообщение", false, now
    );
    mockPrivateMessageService->setGetMessageResult(msg);

    auto response = makeGetRequest("/api/v1/private-messages/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getGetMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetMessageId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("senderUserId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("receiverUserId")).as_integer(), 200);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Конкретное сообщение"));
}

BOOST_AUTO_TEST_CASE(test_get_private_message_not_found)
{
    mockPrivateMessageService->setGetMessageResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/private-messages/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getGetMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetMessageId(), 999);
}

// ============================================================
// GET /api/v1/private-messages/conversation/{userId} — Переписка
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_conversation_success)
{
    auto response = makeGetRequest("/api/v1/private-messages/conversation/200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getGetConversationCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetConversationUserId1(), 100);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastGetConversationUserId2(), 200);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_conversation_empty)
{
    mockPrivateMessageService->setGetConversationResult({});

    auto response = makeGetRequest("/api/v1/private-messages/conversation/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// POST /api/v1/private-messages — Отправка сообщения
// ============================================================

BOOST_AUTO_TEST_CASE(test_send_private_message_success)
{
    auto now = std::chrono::system_clock::now();
    dto::PrivateMessage sentMsg = MockPrivateMessageService::createTestMessage(
        100, 100, 200, "Привет, как дела?", false, now
    );
    mockPrivateMessageService->setSendMessageResult(sentMsg);

    web::json::value body;
    body[U("receiverUserId")] = web::json::value::number(200);
    body[U("content")] = web::json::value::string(U("Привет, как дела?"));

    auto response = makePostRequest("/api/v1/private-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getSendMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastSendMessageSenderUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("senderUserId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("receiverUserId")).as_integer(), 200);
    BOOST_CHECK_EQUAL(json.at(U("content")).as_string(), U("Привет, как дела?"));
}

BOOST_AUTO_TEST_CASE(test_send_private_message_missing_receiver)
{
    web::json::value body;
    body[U("content")] = web::json::value::string(U("Сообщение без получателя"));

    auto response = makePostRequest("/api/v1/private-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getSendMessageCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_send_private_message_missing_content)
{
    web::json::value body;
    body[U("receiverUserId")] = web::json::value::number(200);

    auto response = makePostRequest("/api/v1/private-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getSendMessageCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_send_private_message_to_self)
{
    web::json::value body;
    body[U("receiverUserId")] = web::json::value::number(100);
    body[U("content")] = web::json::value::string(U("Сообщение самому себе"));

    auto response = makePostRequest("/api/v1/private-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getSendMessageCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_send_private_message_recipient_not_found)
{
    mockPrivateMessageService->setSendMessageResult(std::nullopt);

    web::json::value body;
    body[U("receiverUserId")] = web::json::value::number(999);
    body[U("content")] = web::json::value::string(U("Несуществующему пользователю"));

    auto response = makePostRequest("/api/v1/private-messages", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_send_private_message_requires_auth)
{
    web::json::value body;
    body[U("receiverUserId")] = web::json::value::number(200);
    body[U("content")] = web::json::value::string(U("Без авторизации"));

    auto response = makePostRequest("/api/v1/private-messages", body, "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getSendMessageCallCount(), 0);
}

// ============================================================
// PUT /api/v1/private-messages/{id}/view — Отметка о прочтении
// ============================================================

BOOST_AUTO_TEST_CASE(test_mark_message_as_viewed_success)
{
    services::PrivateMessageResult viewResult;
    viewResult.success = true;
    mockPrivateMessageService->setMarkAsViewedResult(viewResult);

    auto response = makePutRequest("/api/v1/private-messages/1/view", web::json::value()).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getMarkAsViewedCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastMarkAsViewedMessageId(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastMarkAsViewedUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_mark_message_as_viewed_not_found)
{
    services::PrivateMessageResult viewResult;
    viewResult.success = false;
    viewResult.errorCode = 404;
    viewResult.errorMessage = "Message not found";
    mockPrivateMessageService->setMarkAsViewedResult(viewResult);

    auto response = makePutRequest("/api/v1/private-messages/999/view", web::json::value()).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getMarkAsViewedCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastMarkAsViewedMessageId(), 999);
}

BOOST_AUTO_TEST_CASE(test_mark_message_as_viewed_not_receiver)
{
    services::PrivateMessageResult viewResult;
    viewResult.success = false;
    viewResult.errorCode = 403;
    viewResult.errorMessage = "Only receiver can mark as viewed";
    mockPrivateMessageService->setMarkAsViewedResult(viewResult);

    auto response = makePutRequest("/api/v1/private-messages/1/view", web::json::value()).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/private-messages/{id} — Удаление сообщения
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_private_message_success)
{
    services::PrivateMessageResult deleteResult;
    deleteResult.success = true;
    mockPrivateMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/private-messages/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getDeleteMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastDeletedMessageId(), 3);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastDeleteMessageUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_private_message_not_found)
{
    services::PrivateMessageResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Message not found";
    mockPrivateMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/private-messages/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getDeleteMessageCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastDeletedMessageId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_private_message_no_permission)
{
    services::PrivateMessageResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "No permission to delete";
    mockPrivateMessageService->setDeleteMessageResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/private-messages/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_private_message_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/private-messages/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getDeleteMessageCallCount(), 0);
}

// ============================================================
// GET /api/v1/private-messages/unviewed/count — Непрочитанные
// ============================================================

BOOST_AUTO_TEST_CASE(test_count_unviewed_success)
{
    mockPrivateMessageService->setCountUnviewedResult(5);

    auto response = makeGetRequest("/api/v1/private-messages/unviewed/count").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getCountUnviewedCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getLastCountUnviewedUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("count")).as_integer(), 5);
}

BOOST_AUTO_TEST_CASE(test_count_unviewed_zero)
{
    mockPrivateMessageService->setCountUnviewedResult(0);

    auto response = makeGetRequest("/api/v1/private-messages/unviewed/count").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("count")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_count_unviewed_requires_auth)
{
    auto response = makeGetRequest("/api/v1/private-messages/unviewed/count", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPrivateMessageService->getCountUnviewedCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_private_message_lifecycle)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockPrivateMessageService->reset();

    auto now = std::chrono::system_clock::now();

    // 1. Отправка сообщения
    dto::PrivateMessage sentMsg = MockPrivateMessageService::createTestMessage(
        200, 100, 300, "Привет, это тест!", false, now
    );
    mockPrivateMessageService->setSendMessageResult(sentMsg);

    web::json::value sendBody;
    sendBody[U("receiverUserId")] = web::json::value::number(300);
    sendBody[U("content")] = web::json::value::string(U("Привет, это тест!"));

    auto sendResponse = makePostRequest("/api/v1/private-messages", sendBody).get();
    BOOST_CHECK_EQUAL(sendResponse.status_code(), status_codes::Created);
    auto sendJson = sendResponse.extract_json().get();
    int64_t newMsgId = sendJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newMsgId, 0);

    // 2. Чтение сообщения (как получатель)
    mockPrivateMessageService->setGetMessageResult(sentMsg);
    auto getResponse = makeGetRequest("/api/v1/private-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Отметка о прочтении
    services::PrivateMessageResult viewResult;
    viewResult.success = true;
    mockPrivateMessageService->setMarkAsViewedResult(viewResult);
    auto viewResponse = makePutRequest(
                            "/api/v1/private-messages/" + std::to_string(newMsgId) + "/view",
                            web::json::value()
    )
                            .get();
    BOOST_CHECK_EQUAL(viewResponse.status_code(), status_codes::OK);

    // 4. Получение списка сообщений
    services::PrivateMessagesPage listPage;
    dto::PrivateMessage viewedMsg = sentMsg;
    viewedMsg.isViewed = true;
    listPage.messages = { viewedMsg };
    listPage.totalCount = 1;
    mockPrivateMessageService->setGetMessagesResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/private-messages").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Проверка непрочитанных
    mockPrivateMessageService->setCountUnviewedResult(0);
    auto countResponse = makeGetRequest("/api/v1/private-messages/unviewed/count").get();
    BOOST_CHECK_EQUAL(countResponse.status_code(), status_codes::OK);
    auto countJson = countResponse.extract_json().get();
    BOOST_CHECK_EQUAL(countJson.at(U("count")).as_integer(), 0);

    // 6. Удаление сообщения
    services::PrivateMessageResult deleteResult;
    deleteResult.success = true;
    mockPrivateMessageService->setDeleteMessageResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/private-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 7. Проверка после удаления
    mockPrivateMessageService->setGetMessageResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/private-messages/" + std::to_string(newMsgId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
