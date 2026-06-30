#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_special_day_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct SpecialDaysTestFixture
{
    SpecialDaysTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockSpecialDayService = std::make_shared<MockSpecialDayService>();

        // Супер-админ для изменения календаря
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultSpecialDayService();

        server = std::make_unique<RestServer>("127.0.0.1", 18122);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setSpecialDayService(mockSpecialDayService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultSpecialDayService()
    {
        // Настройка страницы с особыми днями
        services::SpecialDaysPage testPage;

        dto::SpecialDay day1 = MockSpecialDayService::createTestSpecialDay(1, "2024-01-01", false);
        dto::SpecialDay day2 = MockSpecialDayService::createTestSpecialDay(2, "2024-03-08", true, "09:00", "15:00", 30);
        dto::SpecialDay day3 = MockSpecialDayService::createTestSpecialDay(3, "2024-05-01", false);

        testPage.days = { day1, day2, day3 };
        testPage.totalCount = 3;
        mockSpecialDayService->setGetSpecialDaysResult(testPage);
        mockSpecialDayService->setGetSpecialDayResult(day1);

        dto::SpecialDay newDay = MockSpecialDayService::createTestSpecialDay(100, "2024-12-31", true, "09:00", "14:00", 30);
        mockSpecialDayService->setCreateSpecialDayResult(newDay);

        dto::SpecialDay updatedDay = day1;
        updatedDay.isWorkDay = true;
        updatedDay.beginWorkTime = "10:00";
        updatedDay.endWorkTime = "16:00";
        updatedDay.breakDuration = 45;
        mockSpecialDayService->setUpdateSpecialDayResult(updatedDay);

        services::SpecialDayResult deleteResult;
        deleteResult.success = true;
        mockSpecialDayService->setDeleteSpecialDayResult(deleteResult);
    }

    ~SpecialDaysTestFixture()
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
    std::shared_ptr<MockSpecialDayService> mockSpecialDayService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(SpecialDaysCrudTestSuite, SpecialDaysTestFixture)

// ============================================================
// GET /api/v1/special-days — Получение списка особых дней
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_special_days_returns_list)
{
    auto response = makeGetRequest("/api/v1/special-days").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getGetSpecialDaysCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDaysUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_special_days_with_pagination)
{
    services::SpecialDaysPage emptyPage;
    mockSpecialDayService->setGetSpecialDaysResult(emptyPage);

    auto response = makeGetRequest("/api/v1/special-days?page=2&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDaysPage(), 2);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDaysPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_special_days_filter_by_year)
{
    services::SpecialDaysPage filteredPage;
    dto::SpecialDay day = MockSpecialDayService::createTestSpecialDay(10, "2024-07-04", false);
    filteredPage.days = { day };
    filteredPage.totalCount = 1;
    mockSpecialDayService->setGetSpecialDaysResult(filteredPage);

    auto response = makeGetRequest("/api/v1/special-days?year=2024").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockSpecialDayService->getLastGetSpecialDaysYear().has_value());
    BOOST_CHECK_EQUAL(*mockSpecialDayService->getLastGetSpecialDaysYear(), 2024);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_special_days_filter_by_year_and_month)
{
    services::SpecialDaysPage filteredPage;
    dto::SpecialDay day = MockSpecialDayService::createTestSpecialDay(20, "2024-05-15", false);
    filteredPage.days = { day };
    filteredPage.totalCount = 1;
    mockSpecialDayService->setGetSpecialDaysResult(filteredPage);

    auto response = makeGetRequest("/api/v1/special-days?year=2024&month=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockSpecialDayService->getLastGetSpecialDaysYear().has_value());
    BOOST_CHECK_EQUAL(*mockSpecialDayService->getLastGetSpecialDaysYear(), 2024);
    BOOST_REQUIRE(mockSpecialDayService->getLastGetSpecialDaysMonth().has_value());
    BOOST_CHECK_EQUAL(*mockSpecialDayService->getLastGetSpecialDaysMonth(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_special_days_requires_auth)
{
    auto response = makeGetRequest("/api/v1/special-days", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getGetSpecialDaysCallCount(), 0);
}

// ============================================================
// GET /api/v1/special-days/{id} — Получение особого дня по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_special_day_by_id_success)
{
    dto::SpecialDay day = MockSpecialDayService::createTestSpecialDay(42, "2024-04-20", false);
    mockSpecialDayService->setGetSpecialDayResult(day);

    auto response = makeGetRequest("/api/v1/special-days/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getGetSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDayId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("date")).as_integer(), common::timePointToSeconds(common::stringToDateTime("2024-04-20 00:00:00")));
}

BOOST_AUTO_TEST_CASE(test_get_special_day_not_found)
{
    mockSpecialDayService->setGetSpecialDayResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/special-days/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getGetSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDayId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_special_day_invalid_id)
{
    auto response = makeGetRequest("/api/v1/special-days/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/special-days — Создание особого дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_special_day_success)
{
    dto::SpecialDay createdDay = MockSpecialDayService::createTestSpecialDay(100, "2024-12-25", false);
    mockSpecialDayService->setCreateSpecialDayResult(createdDay);

    web::json::value body;
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-12-25 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/special-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getCreateSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastCreateSpecialDayUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("isWorkDay")).as_bool(), false);
}

BOOST_AUTO_TEST_CASE(test_create_special_day_with_custom_time)
{
    dto::SpecialDay createdDay = MockSpecialDayService::createTestSpecialDay(
        101, "2024-12-31", true, "09:00", "14:00", 30
    );
    mockSpecialDayService->setCreateSpecialDayResult(createdDay);

    web::json::value body;
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-12-31 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(true);
    body[U("beginWorkTime")] = web::json::value::string(U("09:00"));
    body[U("endWorkTime")] = web::json::value::string(U("14:00"));
    body[U("breakDuration")] = web::json::value::number(30);

    auto response = makePostRequest("/api/v1/special-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("isWorkDay")).as_bool(), true);
    BOOST_CHECK_EQUAL(json.at(U("beginWorkTime")).as_string(), U("09:00"));
    BOOST_CHECK_EQUAL(json.at(U("endWorkTime")).as_string(), U("14:00"));
}

BOOST_AUTO_TEST_CASE(test_create_special_day_missing_date)
{
    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/special-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getCreateSpecialDayCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_special_day_duplicate)
{
    mockSpecialDayService->setCreateSpecialDayResult(std::nullopt);

    web::json::value body;
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-01-01 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/special-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_create_special_day_insufficient_permissions)
{
    // Обычный пользователь (не супер-админ)
    mockAuthMiddleware->setValidateRequestResult(true, "100");
    mockSpecialDayService->setCreateSpecialDayResult(std::nullopt);

    web::json::value body;
    body[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-11-04 00:00:00"))
    );
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/special-days", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// PUT /api/v1/special-days/{id} — Обновление особого дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_special_day_success)
{
    dto::SpecialDay updatedDay = MockSpecialDayService::createTestSpecialDay(1, "2024-01-01", true, "10:00", "16:00", 45);
    mockSpecialDayService->setUpdateSpecialDayResult(updatedDay);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);
    body[U("beginWorkTime")] = web::json::value::string(U("10:00"));
    body[U("endWorkTime")] = web::json::value::string(U("16:00"));
    body[U("breakDuration")] = web::json::value::number(45);

    auto response = makePutRequest("/api/v1/special-days/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getUpdateSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastUpdateSpecialDayId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("isWorkDay")).as_bool(), true);
    BOOST_CHECK_EQUAL(json.at(U("beginWorkTime")).as_string(), U("10:00"));
    BOOST_CHECK_EQUAL(json.at(U("endWorkTime")).as_string(), U("16:00"));
}

BOOST_AUTO_TEST_CASE(test_update_special_day_not_found)
{
    // Настраиваем: день не найден при обновлении И при получении
    mockSpecialDayService->setUpdateSpecialDayResult(std::nullopt);
    mockSpecialDayService->setGetSpecialDayResult(std::nullopt);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/special-days/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getUpdateSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getGetSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastGetSpecialDayId(), 999);
}


BOOST_AUTO_TEST_CASE(test_update_special_day_invalid_data)
{
    // Настраиваем: день существует, но обновление не удаётся из-за невалидных данных
    // Для этого сервис должен вернуть std::nullopt, но getSpecialDay должен вернуть существующий день
    dto::SpecialDay existingDay = MockSpecialDayService::createTestSpecialDay(1, "2024-01-01", false);
    mockSpecialDayService->setGetSpecialDayResult(existingDay);
    
    // updateSpecialDay возвращает std::nullopt (ошибка валидации)
    mockSpecialDayService->setUpdateSpecialDayResult(std::nullopt);

    // Передаём невалидные данные: рабочий день без времени
    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/special-days/1", body).get();

    // Так как update вернул nullopt, но getSpecialDay вернул существующий день,
    // код должен вернуть 403 (недостаточно прав для обновления)
    // Или в зависимости от реализации - 400 (Bad Request)
    // В нашем случае сервис возвращает std::nullopt из-за ошибки валидации,
    // а обработчик не знает причину, поэтому возвращает 403
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// DELETE /api/v1/special-days/{id} — Удаление особого дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_special_day_success)
{
    services::SpecialDayResult deleteResult;
    deleteResult.success = true;
    mockSpecialDayService->setDeleteSpecialDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/special-days/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getDeleteSpecialDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getLastDeletedSpecialDayId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_special_day_not_found)
{
    services::SpecialDayResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Special day not found";
    mockSpecialDayService->setDeleteSpecialDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/special-days/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockSpecialDayService->getDeleteSpecialDayCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_special_day_insufficient_permissions)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    services::SpecialDayResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockSpecialDayService->setDeleteSpecialDayResult(deleteResult);

    auto response = makeDeleteRequest("/api/v1/special-days/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_special_day_lifecycle)
{
    // 1. Создание особого дня
    dto::SpecialDay newDay = MockSpecialDayService::createTestSpecialDay(200, "2024-10-01", true, "09:00", "18:00", 60);
    mockSpecialDayService->setCreateSpecialDayResult(newDay);

    web::json::value createBody;
    createBody[U("date")] = web::json::value::number(
        common::timePointToSeconds(common::stringToDateTime("2024-10-01 00:00:00"))
    );
    createBody[U("isWorkDay")] = web::json::value::boolean(true);
    createBody[U("beginWorkTime")] = web::json::value::string(U("09:00"));
    createBody[U("endWorkTime")] = web::json::value::string(U("18:00"));
    createBody[U("breakDuration")] = web::json::value::number(60);

    auto createResponse = makePostRequest("/api/v1/special-days", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);
    auto createJson = createResponse.extract_json().get();
    int64_t newId = createJson.at(U("id")).as_integer();

    // 2. Чтение созданного дня
    mockSpecialDayService->setGetSpecialDayResult(newDay);
    auto getResponse = makeGetRequest("/api/v1/special-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление дня
    dto::SpecialDay updatedDay = newDay;
    updatedDay.isWorkDay = false;
    mockSpecialDayService->setUpdateSpecialDayResult(updatedDay);

    web::json::value updateBody;
    updateBody[U("isWorkDay")] = web::json::value::boolean(false);

    auto updateResponse = makePutRequest("/api/v1/special-days/" + std::to_string(newId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Получение списка
    services::SpecialDaysPage listPage;
    listPage.days = { updatedDay };
    listPage.totalCount = 1;
    mockSpecialDayService->setGetSpecialDaysResult(listPage);
    auto listResponse = makeGetRequest("/api/v1/special-days").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 5. Удаление дня
    services::SpecialDayResult deleteResult;
    deleteResult.success = true;
    mockSpecialDayService->setDeleteSpecialDayResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/api/v1/special-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 6. Проверка после удаления
    mockSpecialDayService->setGetSpecialDayResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/special-days/" + std::to_string(newId)).get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
