#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_standard_day_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct StandardDaysTestFixture
{
    StandardDaysTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockStandardDayService = std::make_shared<MockStandardDayService>();

        // Супер-админ для изменения календаря
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultStandardDayService();

        server = std::make_unique<RestServer>("127.0.0.1", 18121);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setStandardDayService(mockStandardDayService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultStandardDayService()
    {
        // Настройка списка стандартных дней (все 7 дней недели)
        std::vector<dto::StandardDay> days;

        for (int i = 0; i < 7; ++i)
        {
            dto::StandardDay day;
            day.id = i + 1;
            day.weekDayNumber = i;
            day.weekOrder = 0;

            if (i == 0 || i == 6) // Воскресенье и суббота - выходные
            {
                day.isWorkDay = false;
                day.beginWorkTime = std::nullopt;
                day.endWorkTime = std::nullopt;
                day.breakDuration = 0;
            }
            else // Будние дни - рабочие
            {
                day.isWorkDay = true;
                day.beginWorkTime = "09:00";
                day.endWorkTime = "18:00";
                day.breakDuration = 60;
            }

            days.push_back(day);
        }

        mockStandardDayService->setGetAllStandardDaysResult(days);

        // Настройка отдельного дня
        dto::StandardDay monday;
        monday.id = 2;
        monday.weekDayNumber = 1;
        monday.weekOrder = 0;
        monday.isWorkDay = true;
        monday.beginWorkTime = "09:00";
        monday.endWorkTime = "18:00";
        monday.breakDuration = 60;
        mockStandardDayService->setGetStandardDayByWeekDayResult(monday);

        // Настройка обновления
        services::StandardDayResult updateResult;
        updateResult.success = true;
        mockStandardDayService->setUpdateStandardDayResult(updateResult);
    }

    ~StandardDaysTestFixture()
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

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockUserService> mockUserService;
    std::shared_ptr<MockStandardDayService> mockStandardDayService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(StandardDaysCrudTestSuite, StandardDaysTestFixture)

// ============================================================
// GET /api/v1/standard-days — Получение всех стандартных дней
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_standard_days_returns_all_days)
{
    auto response = makeGetRequest("/api/v1/standard-days").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStandardDayService->getGetAllStandardDaysCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.is_array());
    BOOST_CHECK_EQUAL(json.as_array().size(), 7);

    // Проверяем, что все дни недели присутствуют
    for (const auto& item : json.as_array())
    {
        int weekDay = item.at(U("weekDayNumber")).as_integer();
        BOOST_CHECK(weekDay >= 0 && weekDay <= 6);

        if (weekDay == 0 || weekDay == 6)
        {
            BOOST_CHECK_EQUAL(item.at(U("isWorkDay")).as_bool(), false);
        }
        else
        {
            BOOST_CHECK_EQUAL(item.at(U("isWorkDay")).as_bool(), true);
            BOOST_CHECK_EQUAL(item.at(U("beginWorkTime")).as_string(), U("09:00"));
            BOOST_CHECK_EQUAL(item.at(U("endWorkTime")).as_string(), U("18:00"));
            BOOST_CHECK_EQUAL(item.at(U("breakDuration")).as_integer(), 60);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_get_standard_days_requires_auth)
{
    auto response = makeGetRequest("/api/v1/standard-days", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockStandardDayService->getGetAllStandardDaysCallCount(), 0);
}

// ============================================================
// GET /api/v1/standard-days/{weekDayNumber} — Получение дня по номеру
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_standard_day_by_week_day_success)
{
    auto response = makeGetRequest("/api/v1/standard-days/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockStandardDayService->getGetStandardDayByWeekDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockStandardDayService->getLastWeekDayNumber(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("weekDayNumber")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("isWorkDay")).as_bool(), true);
    BOOST_CHECK_EQUAL(json.at(U("beginWorkTime")).as_string(), U("09:00"));
    BOOST_CHECK_EQUAL(json.at(U("endWorkTime")).as_string(), U("18:00"));
}

BOOST_AUTO_TEST_CASE(test_get_standard_day_invalid_week_day)
{
    // Номер дня недели должен быть в диапазоне 0-6
    auto response = makeGetRequest("/api/v1/standard-days/7").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockStandardDayService->getGetStandardDayByWeekDayCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_standard_day_not_found)
{
    mockStandardDayService->setGetStandardDayByWeekDayResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/standard-days/5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockStandardDayService->getGetStandardDayByWeekDayCallCount(), 1);
}

// ============================================================
// PUT /api/v1/standard-days/{weekDayNumber} — Обновление дня
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_standard_day_success)
{
    services::StandardDayResult result;
    result.success = true;
    mockStandardDayService->setUpdateStandardDayResult(result);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(false);
    body[U("beginWorkTime")] = web::json::value::null();
    body[U("endWorkTime")] = web::json::value::null();
    body[U("breakDuration")] = web::json::value::number(0);

    auto response = makePutRequest("/api/v1/standard-days/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockStandardDayService->getUpdateStandardDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockStandardDayService->getLastUpdateWeekDayNumber(), 1);
    BOOST_CHECK_EQUAL(mockStandardDayService->getLastUpdateUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_standard_day_partial)
{
    services::StandardDayResult result;
    result.success = true;
    mockStandardDayService->setUpdateStandardDayResult(result);

    // Обновляем только isWorkDay
    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePutRequest("/api/v1/standard-days/3", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockStandardDayService->getUpdateStandardDayCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_standard_day_missing_week_day)
{
    services::StandardDayResult result;
    result.success = true;
    mockStandardDayService->setUpdateStandardDayResult(result);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/standard-days/abc", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockStandardDayService->getUpdateStandardDayCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_update_standard_day_insufficient_permissions)
{
    // Обычный пользователь (не супер-админ)
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    services::StandardDayResult result;
    result.success = false;
    result.errorCode = 403;
    result.errorMessage = "Insufficient permissions";
    mockStandardDayService->setUpdateStandardDayResult(result);

    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePutRequest("/api/v1/standard-days/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_update_standard_day_invalid_data)
{
    services::StandardDayResult result;
    result.success = false;
    result.errorCode = 400;
    result.errorMessage = "Invalid data";
    mockStandardDayService->setUpdateStandardDayResult(result);

    // Передаём невалидные данные: рабочий день без времени
    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/standard-days/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
}

BOOST_AUTO_TEST_CASE(test_update_standard_day_requires_auth)
{
    web::json::value body;
    body[U("isWorkDay")] = web::json::value::boolean(false);

    auto response = makePutRequest("/api/v1/standard-days/1", body, "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockStandardDayService->getUpdateStandardDayCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_standard_day_lifecycle)
{
    // 1. Получение всех дней
    auto listResponse = makeGetRequest("/api/v1/standard-days").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);
    auto listJson = listResponse.extract_json().get();
    BOOST_CHECK_EQUAL(listJson.as_array().size(), 7);

    // 2. Получение конкретного дня
    auto getResponse = makeGetRequest("/api/v1/standard-days/1").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);
    auto getJson = getResponse.extract_json().get();
    BOOST_CHECK_EQUAL(getJson.at(U("weekDayNumber")).as_integer(), 1);

    // 3. Обновление дня
    services::StandardDayResult updateResult;
    updateResult.success = true;
    mockStandardDayService->setUpdateStandardDayResult(updateResult);

    web::json::value updateBody;
    updateBody[U("isWorkDay")] = web::json::value::boolean(false);

    auto updateResponse = makePutRequest("/api/v1/standard-days/1", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::NoContent);

    // 4. Проверка что обновление было применено (через mock)
    BOOST_CHECK_EQUAL(mockStandardDayService->getUpdateStandardDayCallCount(), 1);
    BOOST_CHECK_EQUAL(mockStandardDayService->getLastUpdateWeekDayNumber(), 1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
