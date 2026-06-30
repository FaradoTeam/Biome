#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_user_day_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct UserDaysTestFixture
{
    UserDaysTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockUserDayService = std::make_shared<MockUserDayService>();

        // Обычный пользователь (не супер-админ)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных по умолчанию
        setupDefaultUserDayService();

        server = std::make_unique<RestServer>("127.0.0.1", 18123);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setUserDayService(mockUserDayService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultUserDayService()
    {
        // Настройка страницы с пользовательскими днями
        services::UserDaysPage testPage;

        dto::UserDay day1 = MockUserDayService::createTestUserDay(1, 100, "2024-01-01", false, "", "", 0, "Новогодние каникулы");
        dto::UserDay day2 = MockUserDayService::createTestUserDay(2, 100, "2024-01-02", false, "", "", 0, "Новогодние каникулы");
        dto::UserDay day3 = MockUserDayService::createTestUserDay(3, 101, "2024-02-15", false, "", "", 0, "Больничный");

        testPage.days = { day1, day2, day3 };
        testPage.totalCount = 3;
        mockUserDayService->setGetUserDaysResult(testPage);
        mockUserDayService->setGetUserDayResult(day1);

        dto::UserDay newDay = MockUserDayService::createTestUserDay(100, 100, "2024-03-08", false, "", "", 0, "Отгул");
        mockUserDayService->setCreateUserDayResult(newDay);

        dto::UserDay updatedDay = day1;
        updatedDay.isWorkDay = true;
        updatedDay.beginWorkTime = "09:00";
        updatedDay.endWorkTime = "13:00";
        updatedDay.breakDuration = 0;
        updatedDay.description = "Сокращённый день";
        mockUserDayService->setUpdateUserDayResult(updatedDay);

        services::UserDayResult deleteResult;
        deleteResult.success = true;
        mockUserDayService->setDeleteUserDayResult(deleteResult);

        // Настройка getByUserAndDate
        mockUserDayService->setGetUserDayByUserAndDateResult(day1);
    }

    ~UserDaysTestFixture()
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

    pplx::task<web::http::http_response> makePutRequest(
        const std::string& path,
        const web::json::value& body,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18123"));
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
    std::shared_ptr<MockUserDayService> mockUserDayService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(UserDaysCrudTestSuite, UserDaysTestFixture)

// ============================================================
// GET /api/v1/user-days — Получение списка пользовательских дней
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_days_returns_list)
{
    auto response = makeGetRequest("/api/v1/user-days").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDaysCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastGetUserDaysUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_user_days_filter_by_user_id)
{
    services::UserDaysPage filteredPage;
    dto::UserDay day = MockUserDayService::createTestUserDay(10, 100, "2024-02-20", false, "", "", 0, "Отпуск");
    filteredPage.days = { day };
    filteredPage.totalCount = 1;
    mockUserDayService->setGetUserDaysResult(filteredPage);

    auto response = makeGetRequest("/api/v1/user-days?userId=100").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserDayService->getLastGetUserDaysFilterUserId().has_value());
    BOOST_CHECK_EQUAL(*mockUserDayService->getLastGetUserDaysFilterUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("userId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_user_days_filter_by_date_range)
{
    services::UserDaysPage filteredPage;
    dto::UserDay day = MockUserDayService::createTestUserDay(20, 100, "2024-01-15", false, "", "", 0, "Отдых");
    filteredPage.days = { day };
    filteredPage.totalCount = 1;
    mockUserDayService->setGetUserDaysResult(filteredPage);

    auto dateFrom = common::timePointToSeconds(common::stringToDateTime("2024-01-01 00:00:00"));
    auto dateTo = common::timePointToSeconds(common::stringToDateTime("2024-01-31 00:00:00"));

    auto response = makeGetRequest(
                        "/api/v1/user-days?dateFrom=" + std::to_string(dateFrom) + "&dateTo=" + std::to_string(dateTo)
    )
                        .get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockUserDayService->getLastGetUserDaysDateFrom().has_value());
    BOOST_REQUIRE(mockUserDayService->getLastGetUserDaysDateTo().has_value());
}

BOOST_AUTO_TEST_CASE(test_get_user_days_other_user_denied)
{
    // Обычный пользователь не может видеть дни других пользователей
    services::UserDaysPage emptyPage;
    emptyPage.totalCount = 0;
    mockUserDayService->setGetUserDaysResult(emptyPage);

    auto response = makeGetRequest("/api/v1/user-days?userId=101").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    // Сервис должен вернуть пустой результат
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_user_days_requires_auth)
{
    auto response = makeGetRequest("/api/v1/user-days", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDaysCallCount(), 0);
}

// ============================================================
// GET /api/v1/user-days/{id} — Получение пользовательского дня по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_day_by_id_success)
{
    dto::UserDay day = MockUserDayService::createTestUserDay(42, 100, "2024-03-01", false, "", "", 0, "Отпуск");
    mockUserDayService->setGetUserDayResult(day);

    auto response = makeGetRequest("/api/v1/user-days/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastGetUserDayId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Отпуск"));
}

BOOST_AUTO_TEST_CASE(test_get_user_day_not_found)
{
    mockUserDayService->setGetUserDayResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/user-days/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDayCallCount(), 1);
}

// ============================================================
// GET /api/v1/users/{userId}/days/{date} — Получение дня по пользователю и дате
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_user_day_by_user_and_date_success)
{
    auto date = common::timePointToSeconds(common::stringToDateTime("2024-01-01 00:00:00"));

    auto response = makeGetRequest(
                        "/api/v1/users/100/days/" + std::to_string(date)
    )
                        .get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDayByUserAndDateCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastGetUserDayByUserAndDateUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Новогодние каникулы"));
}

BOOST_AUTO_TEST_CASE(test_get_user_day_by_user_and_date_not_found)
{
    mockUserDayService->setGetUserDayByUserAndDateResult(std::nullopt);

    auto date = common::timePointToSeconds(common::stringToDateTime("2024-06-01 00:00:00"));

    auto response = makeGetRequest(
                        "/api/v1/users/100/days/" + std::to_string(date)
    )
                        .get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/user-days — Создание пользовательского дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_user_day_success)
{
    dto::UserDay createdDay = MockUserDayService::createTestUserDay(100, 100, "2024-03-08", false, "", "", 0, "Отгул");
    mockUserDayService->setCreateUserDayResult(createdDay);

    web::json::value body;
    body[U("userId")] = web::json::value::number(100);
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-03-08 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);
    body[U("description")] = web::json::value::string(U("Отгул"));

    auto response = makePostRequest("/api/v1/user-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockUserDayService->getCreateUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastCreateUserDayUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("userId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Отгул"));
}

BOOST_AUTO_TEST_CASE(test_create_user_day_for_another_user_denied)
{
    // Пользователь пытается создать день для другого пользователя
    mockUserDayService->setCreateUserDayResult(std::nullopt);

    web::json::value body;
    body[U("userId")] = web::json::value::number(101);
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-03-08 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_missing_user_id)
{
    web::json::value body;
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-03-08 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserDayService->getCreateUserDayCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_missing_date)
{
    web::json::value body;
    body[U("userId")] = web::json::value::number(100);
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockUserDayService->getCreateUserDayCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_user_day_duplicate)
{
    mockUserDayService->setCreateUserDayResult(std::nullopt);

    web::json::value body;
    body[U("userId")] = web::json::value::number(100);
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-01-01 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/user-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/user-days/{id} — Обновление пользовательского дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_user_day_success)
{
    dto::UserDay updatedDay = MockUserDayService::createTestUserDay(1, 100, "2024-01-01", true, "09:00", "13:00", 0, "Сокращённый день");
    mockUserDayService->setUpdateUserDayResult(updatedDay);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);
    body[U("beginWorkTime")] = web::json::value::string(U("09:00"));
    body[U("endWorkTime")] = web::json::value::string(U("13:00"));
    body[U("breakDuration")] = web::json::value::number(0);
    body[U("description")] = web::json::value::string(U("Сокращённый день"));

    auto response = makePutRequest("/api/v1/user-days/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockUserDayService->getUpdateUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastUpdateUserDayId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("isWorkDay")).as_bool(), true);
    BOOST_CHECK_EQUAL(json.at(U("beginWorkTime")).as_string(), U("09:00"));
    BOOST_CHECK_EQUAL(json.at(U("endWorkTime")).as_string(), U("13:00"));
    BOOST_CHECK_EQUAL(json.at(U("description")).as_string(), U("Сокращённый день"));
}

BOOST_AUTO_TEST_CASE(test_update_user_day_not_found)
{
    // Настраиваем: день не найден при обновлении И при получении
    mockUserDayService->setUpdateUserDayResult(std::nullopt);
    mockUserDayService->setGetUserDayResult(std::nullopt);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/user-days/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserDayService->getUpdateUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getGetUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastGetUserDayId(), 999);
}

BOOST_AUTO_TEST_CASE(test_update_user_day_another_user_denied)
{
    // Пытаемся обновить день другого пользователя
    mockUserDayService->setUpdateUserDayResult(std::nullopt);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/user-days/3", body).get(); // день пользователя 101

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/user-days/{id} — Удаление пользовательского дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_user_day_success)
{
    services::UserDayResult deleteResult;
    deleteResult.success = true;
    mockUserDayService->setDeleteUserDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-days/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserDayService->getDeleteUserDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastDeletedUserDayId(), 2);
}

BOOST_AUTO_TEST_CASE(test_delete_user_day_not_found)
{
    services::UserDayResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "User day not found";
    mockUserDayService->setDeleteUserDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-days/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockUserDayService->getDeleteUserDayCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_user_day_another_user_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    services::UserDayResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockUserDayService->setDeleteUserDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/user-days/3").get(); // день пользователя 101

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/users/{userId}/days — Удаление всех дней пользователя
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_user_days_by_user_success)
{
    mockUserDayService->setDeleteUserDaysByUserResult(3);

    auto response = makeDeleteRequest("/api/v1/users/100/days").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserDayService->getDeleteUserDaysByUserCallCount(), 1);
    BOOST_CHECK_EQUAL(mockUserDayService->getLastDeleteUserDaysByUserUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_user_days_by_user_another_user_denied)
{
    mockUserDayService->setDeleteUserDaysByUserResult(0);

    auto response = makeDeleteRequest("/api/v1/users/101/days").get();

    // Должен вернуть NoContent, но с 0 удалённых записей
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockUserDayService->getDeleteUserDaysByUserCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_user_days_by_user_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/users/100/days", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockUserDayService->getDeleteUserDaysByUserCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_user_day_lifecycle)
{
    // 1. Создание пользовательского дня
    dto::UserDay newDay = MockUserDayService::createTestUserDay(200, 100, "2024-04-01", false, "", "", 0, "Отпуск");
    mockUserDayService->setCreateUserDayResult(newDay);

    web::json::value createBody;
    createBody[U("userId")] = web::json::value::number(100);
    createBody[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-04-01 00:00:00"))
    );
    createBody[U("isWorkDay")] = web::json::value::boolean(false);
    createBody[U("description")] = web::json::value::string(U("Отпуск"));

    auto createResponse = makePostRequest("/api/v1/user-days", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newId = createJson.at(U("id")).as_integer();

    // 2. Чтение созданного дня
    mockUserDayService->setGetUserDayResult(newDay);
    auto getResponse = makeGetRequest("/api/v1/user-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление дня
    dto::UserDay updatedDay = newDay;
    updatedDay.isWorkDay = true;
    updatedDay.beginWorkTime = "09:00";
    updatedDay.endWorkTime = "14:00";
    updatedDay.description = "Сокращённый день";
    mockUserDayService->setUpdateUserDayResult(updatedDay);

    web::json::value updateBody;
    updateBody[U("isWorkDay")] = web::json::value::boolean(true);
    updateBody[U("beginWorkTime")] = web::json::value::string(U("09:00"));
    updateBody[U("endWorkTime")] = web::json::value::string(U("14:00"));
    updateBody[U("description")] = web::json::value::string(U("Сокращённый день"));

    auto updateResponse = makePutRequest("/api/v1/user-days/" + std::to_string(newId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Получение по пользователю и дате
    auto date = common::timePointToSeconds(common::stringToDateTime("2024-04-01 00:00:00"));
    auto byUserResponse = makeGetRequest(
                              "/api/v1/users/100/days/" + std::to_string(date)
    )
                              .get();
    BOOST_CHECK_EQUAL(byUserResponse.status_code(), status_codes::OK);

    // 5. Получение списка
    services::UserDaysPage listPage;
    listPage.days = { updatedDay };
    listPage.totalCount = 1;
    mockUserDayService->setGetUserDaysResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/user-days").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 6. Удаление дня
    services::UserDayResult deleteResult;
    deleteResult.success = true;
    mockUserDayService->setDeleteUserDayResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/user-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 7. Проверка после удаления
    mockUserDayService->setGetUserDayResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/user-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
