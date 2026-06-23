#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_notification_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct UserNotificationsTestFixture
{
    UserNotificationsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockUserNotificationService = std::make_shared<MockUserNotificationService>();

        // Обычный пользователь с правами
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultUserNotificationService();

        server = std::make_unique<RestServer>("127.0.0.1", 18124);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setUserNotificationService(mockUserNotificationService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultUserNotificationService()
    {
        // Настройка списка подписок
        services::UserNotificationsPage testPage;

        dto::UserNotification notif1 = MockUserNotificationService::createTestNotification(1, 100, 10);
        dto::UserNotification notif2 = MockUserNotificationService::createTestNotification(2, 200, 10);
        dto::UserNotification notif3 = MockUserNotificationService::createTestNotification(3, 100, 20);

        testPage.notifications = { notif1, notif2, notif3 };
        testPage.totalCount = 3;
        mockUserNotificationService->setGetNotificationsResult(testPage);
        mockUserNotificationService->setGetNotificationResult(notif1);

        // Настройка подписки
        dto::UserNotification newNotif = MockUserNotificationService::createTestNotification(100, 100, 30);
        mockUserNotificationService->setSubscribeResult(newNotif);

        // Настройка подписок пользователя
        std::vector<dto::UserNotification> userNotifications = { notif1, notif3 };
        mockUserNotificationService->setGetUserNotificationsResult(userNotifications);

        // Настройка подписчиков
        std::vector<int64_t> subscriberIds = { 100, 200 };
        mockUserNotificationService->setGetSubscriberIdsResult(subscriberIds);

        // Настройка проверки подписки
        mockUserNotificationService->setIsSubscribedResult(true);

        // Настройка отписки
        services::UserNotificationResult unsubscribeResult;
        unsubscribeResult.success = true;
        mockUserNotificationService->setUnsubscribeResult(unsubscribeResult);
    }

    ~UserNotificationsTestFixture()
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
    std::shared_ptr<MockUserNotificationService> mockUserNotificationService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UserNotificationsCrudTestSuite, UserNotificationsTestFixture)

// ============================================================
// GET /api/v1/user-notifications — Получение списка подписок
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_notifications_returns_list)
{
    auto response = makeGetRequest("/api/v1/user-notifications").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getGetNotificationsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetNotificationsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_notifications_with_pagination_params)
{
    services::UserNotificationsPage emptyPage;
    mockUserNotificationService->setGetNotificationsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/user-notifications?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetNotificationsPage(), 3);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetNotificationsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_notifications_filter_by_user)
{
    mockUserNotificationService->reset();

    dto::UserNotification notif = MockUserNotificationService::createTestNotification(10, 200, 10);
    services::UserNotificationsPage filteredPage;
    filteredPage.notifications = { notif };
    filteredPage.totalCount = 1;
    mockUserNotificationService->setGetNotificationsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-notifications?userId=200").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserNotificationService->getLastGetNotificationsFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserNotificationService->getLastGetNotificationsFilterUserId(), 200);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_notifications_filter_by_item)
{
    mockUserNotificationService->reset();

    dto::UserNotification notif = MockUserNotificationService::createTestNotification(20, 100, 42);
    services::UserNotificationsPage filteredPage;
    filteredPage.notifications = { notif };
    filteredPage.totalCount = 1;
    mockUserNotificationService->setGetNotificationsResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-notifications?itemId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserNotificationService->getLastGetNotificationsItemId().has_value());
    BOOST_CHECK_EQUAL(*mockUserNotificationService->getLastGetNotificationsItemId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("itemId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_notifications_requires_auth)
{
    auto response = makeGetRequest("/api/v1/user-notifications", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getGetNotificationsCallCount(), 0);
}

// ============================================================
// GET /api/v1/user-notifications/{id} — Получение подписки по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_notification_by_id_success)
{
    dto::UserNotification notif = MockUserNotificationService::createTestNotification(42, 100, 10);
    mockUserNotificationService->setGetNotificationResult(notif);

    auto response = makeGetRequest("/api/v1/user-notifications/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getGetNotificationCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetNotificationId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_notification_not_found)
{
    mockUserNotificationService->setGetNotificationResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/user-notifications/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getGetNotificationCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetNotificationId(), 999);
}

// ============================================================
// POST /api/v1/user-notifications — Подписка на элемент
// ============================================================

BOOST_AUTO_TEST_CASE(test_subscribe_success)
{
    dto::UserNotification newNotif = MockUserNotificationService::createTestNotification(100, 100, 30);
    mockUserNotificationService->setSubscribeResult(newNotif);

    web::json::value body;
    body[U("itemId")] = web::json::value::number(30);

    auto response = makePostRequest("/api/v1/user-notifications", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getSubscribeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastSubscribeUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("itemId")).as_integer(), 30);
}

BOOST_AUTO_TEST_CASE(test_subscribe_missing_item_id)
{
    web::json::value body;
    body[U("userId")] = web::json::value::number(100);

    auto response = makePostRequest("/api/v1/user-notifications", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getSubscribeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_subscribe_already_exists)
{
    mockUserNotificationService->setSubscribeResult(std::nullopt);

    web::json::value body;
    body[U("itemId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/user-notifications", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Conflict);
}

BOOST_AUTO_TEST_CASE(test_subscribe_requires_auth)
{
    web::json::value body;
    body[U("itemId")] = web::json::value::number(30);

    auto response = makePostRequest("/api/v1/user-notifications", body, "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getSubscribeCallCount(), 0);
}

// ============================================================
// DELETE /api/v1/user-notifications/{id} — Отписка
// ============================================================

BOOST_AUTO_TEST_CASE(test_unsubscribe_success)
{
    services::UserNotificationResult unsubscribeResult;
    unsubscribeResult.success = true;
    mockUserNotificationService->setUnsubscribeResult(unsubscribeResult);

    auto response = makeDeleteRequest("/api/v1/user-notifications/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getUnsubscribeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastUnsubscribeId(), 3);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastUnsubscribeUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_unsubscribe_not_found)
{
    services::UserNotificationResult unsubscribeResult;
    unsubscribeResult.success = false;
    unsubscribeResult.errorCode = 404;
    unsubscribeResult.errorMessage = "Notification not found";
    mockUserNotificationService->setUnsubscribeResult(unsubscribeResult);

    auto response = makeDeleteRequest("/api/v1/user-notifications/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getUnsubscribeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastUnsubscribeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_unsubscribe_no_permission)
{
    services::UserNotificationResult unsubscribeResult;
    unsubscribeResult.success = false;
    unsubscribeResult.errorCode = 403;
    unsubscribeResult.errorMessage = "No permission";
    mockUserNotificationService->setUnsubscribeResult(unsubscribeResult);

    auto response = makeDeleteRequest("/api/v1/user-notifications/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_unsubscribe_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/user-notifications/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getUnsubscribeCallCount(), 0);
}

// ============================================================
// GET /api/v1/items/{itemId}/subscribers — Подписчики элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_subscribers_success)
{
    auto response = makeGetRequest("/api/v1/items/10/subscribers").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getGetSubscriberIdsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetSubscriberIdsItemId(), 10);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastGetSubscriberIdsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 2);
    BOOST_CHECK_EQUAL(json[0].as_integer(), 100);
    BOOST_CHECK_EQUAL(json[1].as_integer(), 200);
}

BOOST_AUTO_TEST_CASE(test_get_subscribers_empty)
{
    mockUserNotificationService->setGetSubscriberIdsResult({});

    auto response = makeGetRequest("/api/v1/items/999/subscribers").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.as_array().size(), 0);
}

// ============================================================
// GET /api/v1/items/{itemId}/subscribed — Проверка подписки
// ============================================================

BOOST_AUTO_TEST_CASE(test_is_subscribed_true)
{
    mockUserNotificationService->setIsSubscribedResult(true);

    auto response = makeGetRequest("/api/v1/items/10/subscribed").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getIsSubscribedCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastIsSubscribedUserId(), 100);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getLastIsSubscribedItemId(), 10);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("subscribed")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_is_subscribed_false)
{
    mockUserNotificationService->setIsSubscribedResult(false);

    auto response = makeGetRequest("/api/v1/items/999/subscribed").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("subscribed")).as_bool(), false);
}

BOOST_AUTO_TEST_CASE(test_is_subscribed_requires_auth)
{
    auto response = makeGetRequest("/api/v1/items/10/subscribed", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserNotificationService->getIsSubscribedCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_notification_lifecycle)
{
    // Очищаем и создаем тестовые данные только для этого теста
    mockUserNotificationService->reset();

    // 1. Подписка на элемент
    dto::UserNotification newNotif = MockUserNotificationService::createTestNotification(200, 100, 50);
    mockUserNotificationService->setSubscribeResult(newNotif);

    web::json::value subscribeBody;
    subscribeBody[U("itemId")] = web::json::value::number(50);

    auto subscribeResponse = makePostRequest("/api/v1/user-notifications", subscribeBody).get();
    BOOST_CHECK_EQUAL(subscribeResponse.status_code(), status_codes::Created);
    auto subscribeJson = subscribeResponse.extract_json().get();
    int64_t newNotifId = subscribeJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newNotifId, 0);

    // 2. Проверка подписки
    mockUserNotificationService->setIsSubscribedResult(true);
    auto checkResponse = makeGetRequest("/api/v1/items/50/subscribed").get();
    BOOST_CHECK_EQUAL(checkResponse.status_code(), status_codes::OK);
    auto checkJson = checkResponse.extract_json().get();
    BOOST_CHECK_EQUAL(checkJson.at(U("subscribed")).as_bool(), true);

    // 3. Получение подписки по ID
    mockUserNotificationService->setGetNotificationResult(newNotif);
    auto getResponse = makeGetRequest("/api/v1/user-notifications/" + std::to_string(newNotifId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 4. Получение списка подписок пользователя
    std::vector<dto::UserNotification> userNotifs = { newNotif };
    mockUserNotificationService->setGetUserNotificationsResult(userNotifs);
    auto userResponse = makeGetRequest("/api/v1/user-notifications").get();
    BOOST_CHECK_EQUAL(userResponse.status_code(), status_codes::OK);

    // 5. Получение подписчиков элемента
    std::vector<int64_t> subscribers = { 100 };
    mockUserNotificationService->setGetSubscriberIdsResult(subscribers);
    auto subsResponse = makeGetRequest("/api/v1/items/50/subscribers").get();
    BOOST_CHECK_EQUAL(subsResponse.status_code(), status_codes::OK);

    // 6. Отписка
    services::UserNotificationResult unsubscribeResult;
    unsubscribeResult.success = true;
    mockUserNotificationService->setUnsubscribeResult(unsubscribeResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/user-notifications/" + std::to_string(newNotifId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 7. Проверка после отписки
    mockUserNotificationService->setGetNotificationResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/user-notifications/" + std::to_string(newNotifId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);

    // 8. Проверка статуса подписки после отписки
    mockUserNotificationService->setIsSubscribedResult(false);
    auto checkAfterDelete = makeGetRequest("/api/v1/items/50/subscribed").get();
    BOOST_CHECK_EQUAL(checkAfterDelete.status_code(), status_codes::OK);
    auto checkAfterJson = checkAfterDelete.extract_json().get();
    BOOST_CHECK_EQUAL(checkAfterJson.at(U("subscribed")).as_bool(), false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
