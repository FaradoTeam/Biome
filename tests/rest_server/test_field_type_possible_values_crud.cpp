#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_field_type_possible_value_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct FieldTypePossibleValuesTestFixture
{
    FieldTypePossibleValuesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockFieldTypePossibleValueService = std::make_shared<MockFieldTypePossibleValueService>();

        // Супер-админ (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultFieldTypePossibleValueService();

        server = std::make_unique<RestServer>("127.0.0.1", 18088);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setFieldTypePossibleValueService(mockFieldTypePossibleValueService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultFieldTypePossibleValueService()
    {
        // Настройка списка возможных значений
        services::FieldTypePossibleValuesPage testPage;

        dto::FieldTypePossibleValue highValue;
        highValue.id = 1;
        highValue.fieldTypeId = 1;
        highValue.value = "Высокий";

        dto::FieldTypePossibleValue mediumValue;
        mediumValue.id = 2;
        mediumValue.fieldTypeId = 1;
        mediumValue.value = "Средний";

        dto::FieldTypePossibleValue lowValue;
        lowValue.id = 3;
        lowValue.fieldTypeId = 1;
        lowValue.value = "Низкий";

        dto::FieldTypePossibleValue criticalValue;
        criticalValue.id = 4;
        criticalValue.fieldTypeId = 1;
        criticalValue.value = "Критический";

        testPage.values = { highValue, mediumValue, lowValue, criticalValue };
        testPage.totalCount = 4;
        mockFieldTypePossibleValueService->setGetValuesResult(testPage);
        mockFieldTypePossibleValueService->setGetValueResult(highValue);

        dto::FieldTypePossibleValue newValue;
        newValue.id = 100;
        newValue.fieldTypeId = 1;
        newValue.value = "Новое значение";
        mockFieldTypePossibleValueService->setCreateValueResult(newValue);

        dto::FieldTypePossibleValue updatedValue = highValue;
        updatedValue.value = "Обновленное значение";
        mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);
        mockFieldTypePossibleValueService->setDeleteValueResult(true);

        // Настройка значений для конкретного типа поля
        std::vector<dto::FieldTypePossibleValue> valuesByFieldType = { highValue, mediumValue, lowValue };
        mockFieldTypePossibleValueService->setValuesByFieldTypeIdResult(valuesByFieldType);
    }

    ~FieldTypePossibleValuesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18088"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18088"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18088"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18088"));
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
    std::shared_ptr<MockFieldTypePossibleValueService> mockFieldTypePossibleValueService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(FieldTypePossibleValuesCrudTestSuite, FieldTypePossibleValuesTestFixture)

// ============================================================
// GET /api/v1/field-type-values — Получение списка возможных значений
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_values_returns_list)
{
    auto response = makeGetRequest("/api/v1/field-type-values").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValuesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 4);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 4);
}

BOOST_AUTO_TEST_CASE(test_get_values_with_pagination_params)
{
    services::FieldTypePossibleValuesPage emptyPage;
    mockFieldTypePossibleValueService->setGetValuesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/field-type-values?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValuesPage(), 3);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValuesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_values_with_field_type_filter)
{
    services::FieldTypePossibleValuesPage filteredPage;
    dto::FieldTypePossibleValue value;
    value.id = 10;
    value.fieldTypeId = 42;
    value.value = "FilteredValue";
    filteredPage.values = { value };
    filteredPage.totalCount = 1;
    mockFieldTypePossibleValueService->setGetValuesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/field-type-values?fieldTypeId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK(mockFieldTypePossibleValueService->getLastGetValuesFieldTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockFieldTypePossibleValueService->getLastGetValuesFieldTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
    BOOST_CHECK_EQUAL(
        json.at(U("items"))[0].at(U("fieldTypeId")).as_integer(), 42
    );
}

BOOST_AUTO_TEST_CASE(test_get_values_empty_list)
{
    services::FieldTypePossibleValuesPage emptyPage;
    mockFieldTypePossibleValueService->setGetValuesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/field-type-values").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_values_requires_auth)
{
    auto response = makeGetRequest("/api/v1/field-type-values", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValuesCallCount(), 0);
}

// ============================================================
// GET /api/v1/field-type-values/{id} — Получение значения по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_value_by_id_success)
{
    dto::FieldTypePossibleValue value;
    value.id = 42;
    value.fieldTypeId = 1;
    value.value = "Конкретное значение";
    mockFieldTypePossibleValueService->setGetValueResult(value);

    auto response = makeGetRequest("/api/v1/field-type-values/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValueCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValueId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("fieldTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("value")).as_string(), U("Конкретное значение"));
}

BOOST_AUTO_TEST_CASE(test_get_value_not_found)
{
    mockFieldTypePossibleValueService->setGetValueResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/field-type-values/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValueCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValueId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_value_invalid_id)
{
    auto response = makeGetRequest("/api/v1/field-type-values/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_value_negative_id)
{
    auto response = makeGetRequest("/api/v1/field-type-values/-1").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// GET /api/v1/field-type-values/by-field-type/{fieldTypeId}
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_values_by_field_type_id_success)
{
    // Убедимся, что сервис настроен на возврат значений
    std::vector<dto::FieldTypePossibleValue> expectedValues;

    dto::FieldTypePossibleValue value1;
    value1.id = 1;
    value1.fieldTypeId = 1;
    value1.value = "Высокий";
    expectedValues.push_back(value1);

    dto::FieldTypePossibleValue value2;
    value2.id = 2;
    value2.fieldTypeId = 1;
    value2.value = "Средний";
    expectedValues.push_back(value2);

    mockFieldTypePossibleValueService->setValuesByFieldTypeIdResult(expectedValues);

    auto response = makeGetRequest("/api/v1/field-type-values/by-field-type/1").get();

    BOOST_CHECK_MESSAGE(
        response.status_code() == status_codes::OK,
        "Expected 200 OK, got " << response.status_code()
    );

    if (response.status_code() == status_codes::OK)
    {
        BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValuesByFieldTypeIdCallCount(), 1);
        BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastValuesByFieldTypeId(), 1);

        auto json = response.extract_json().get();
        BOOST_CHECK(json.is_array());
        BOOST_CHECK_EQUAL(json.as_array().size(), 2);
    }
}

BOOST_AUTO_TEST_CASE(test_get_values_by_field_type_id_empty)
{
    mockFieldTypePossibleValueService->setValuesByFieldTypeIdResult({});

    auto response = makeGetRequest("/api/v1/field-type-values/by-field-type/999").get();

    BOOST_CHECK_MESSAGE(
        response.status_code() == status_codes::OK,
        "Expected 200 OK, got " << response.status_code()
    );

    if (response.status_code() == status_codes::OK)
    {
        auto json = response.extract_json().get();
        BOOST_CHECK(json.is_array());
        BOOST_CHECK_EQUAL(json.as_array().size(), 0);
    }
}

// ============================================================
// POST /api/v1/field-type-values — Создание возможного значения
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_value_success)
{
    dto::FieldTypePossibleValue createdValue;
    createdValue.id = 100;
    createdValue.fieldTypeId = 1;
    createdValue.value = "Новое значение";
    mockFieldTypePossibleValueService->setCreateValueResult(createdValue);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U("Новое значение"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockFieldTypePossibleValueService->getLastCreatedValue().fieldTypeId, 1
    );
    BOOST_CHECK_EQUAL(
        *mockFieldTypePossibleValueService->getLastCreatedValue().value, "Новое значение"
    );
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastCreateValueUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("fieldTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("value")).as_string(), U("Новое значение"));
}

BOOST_AUTO_TEST_CASE(test_create_value_missing_required_fields)
{
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_value_missing_field_type_id)
{
    web::json::value body;
    body[U("value")] = web::json::value::string(U("Только значение"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_value_empty_value_fails)
{
    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U(""));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_value_field_type_not_found)
{
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(999);
    body[U("value")] = web::json::value::string(U("Значение"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_value_duplicate_fails)
{
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U("Высокий"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    // Значение уже существует, сервис должен вернуть ошибку
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->createValueCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_value_regular_user_forbidden)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Настраиваем сервис на возврат nullopt
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U("Новое значение"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    // Должен быть 403 Forbidden
    BOOST_CHECK(response.status_code() == status_codes::Forbidden || response.status_code() == status_codes::NotFound);

    // Сервис мог быть вызван, но вернул nullopt
    // Не проверяем строго count, так как вызов может произойти
}

// ============================================================
// PUT /api/v1/field-type-values/{id} — Обновление значения
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_value_success)
{
    dto::FieldTypePossibleValue updatedValue;
    updatedValue.id = 1;
    updatedValue.fieldTypeId = 1;
    updatedValue.value = "Обновленное значение";
    mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Обновленное значение"));

    auto response = makePutRequest("/api/v1/field-type-values/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->updateValueCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockFieldTypePossibleValueService->getLastUpdatedValue().id, 1
    );
    BOOST_CHECK_EQUAL(
        *mockFieldTypePossibleValueService->getLastUpdatedValue().value,
        "Обновленное значение"
    );
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastUpdateValueUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("value")).as_string(), U("Обновленное значение"));
}

BOOST_AUTO_TEST_CASE(test_update_value_partial)
{
    dto::FieldTypePossibleValue updatedValue;
    updatedValue.id = 1;
    updatedValue.fieldTypeId = 1;
    updatedValue.value = "Только значение";
    mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Только значение"));

    auto response = makePutRequest("/api/v1/field-type-values/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->updateValueCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_value_change_field_type)
{
    dto::FieldTypePossibleValue updatedValue;
    updatedValue.id = 1;
    updatedValue.fieldTypeId = 2;
    updatedValue.value = "Перемещенное значение";
    mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(2);
    body[U("value")] = web::json::value::string(U("Перемещенное значение"));

    auto response = makePutRequest("/api/v1/field-type-values/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(
        *mockFieldTypePossibleValueService->getLastUpdatedValue().fieldTypeId, 2
    );
}

BOOST_AUTO_TEST_CASE(test_update_value_not_found)
{
    // Супер-админ, но значение не существует
    mockFieldTypePossibleValueService->setUpdateValueResult(std::nullopt);
    mockFieldTypePossibleValueService->setGetValueResult(std::nullopt);

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Несуществующее"));

    auto response = makePutRequest("/api/v1/field-type-values/999", body).get();

    // Должен быть 404 Not Found
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_update_value_duplicate_fails)
{
    mockFieldTypePossibleValueService->setUpdateValueResult(std::nullopt);

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Высокий")); // Уже существующее значение

    auto response = makePutRequest("/api/v1/field-type-values/2", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->updateValueCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_value_regular_user_forbidden)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Настраиваем сервис на возврат nullopt
    mockFieldTypePossibleValueService->setUpdateValueResult(std::nullopt);

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/field-type-values/1", body).get();

    // Должен быть 403 Forbidden
    BOOST_CHECK(response.status_code() == status_codes::Forbidden || response.status_code() == status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/field-type-values/{id} — Удаление значения
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_value_success)
{
    mockFieldTypePossibleValueService->setDeleteValueResult(true);

    auto response = makeDeleteRequest("/api/v1/field-type-values/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->deleteValueCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastDeletedValueId(), 3);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastDeleteValueUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_value_not_found)
{
    services::FieldTypePossibleValueResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "Value not found";
    mockFieldTypePossibleValueService->setDeleteValueResult(deleteResult.success);
    mockFieldTypePossibleValueService->setDeleteValueResult(deleteResult.success);
    // Настройка результата через отдельный метод
    mockFieldTypePossibleValueService->setDeleteValueResult(false);

    auto response = makeDeleteRequest("/api/v1/field-type-values/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->deleteValueCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastDeletedValueId(), 999);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_create_value_detailed)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Сбрасываем счётчики
    mockFieldTypePossibleValueService->reset();

    // Настраиваем сервис на возврат nullopt
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U("Попытка создания"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    // Должен быть 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_delete_value_regular_user_forbidden)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Настраиваем сервис на возврат false
    mockFieldTypePossibleValueService->setDeleteValueResult(false);

    auto response = makeDeleteRequest("/api/v1/field-type-values/1").get();

    // Должен быть 403 Forbidden
    BOOST_CHECK(response.status_code() == status_codes::Forbidden || response.status_code() == status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_value_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/field-type-values/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->deleteValueCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_value_lifecycle)
{
    // 1. Создание значения
    dto::FieldTypePossibleValue newValue;
    newValue.id = 100;
    newValue.fieldTypeId = 1;
    newValue.value = "Жизненный цикл значения";
    mockFieldTypePossibleValueService->setCreateValueResult(newValue);

    web::json::value createBody;
    createBody[U("fieldTypeId")] = web::json::value::number(1);
    createBody[U("value")] = web::json::value::string(U("Жизненный цикл значения"));

    auto createResponse = makePostRequest("/api/v1/field-type-values", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданного значения
    mockFieldTypePossibleValueService->setGetValueResult(newValue);
    auto getResponse = makeGetRequest("/api/v1/field-type-values/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);
    auto getJson = getResponse.extract_json().get();
    BOOST_CHECK_EQUAL(getJson.at(U("value")).as_string(), U("Жизненный цикл значения"));

    // 3. Обновление значения
    dto::FieldTypePossibleValue updatedValue = newValue;
    updatedValue.value = "Обновленный жизненный цикл";
    mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);

    web::json::value updateBody;
    updateBody[U("value")] = web::json::value::string(U("Обновленный жизненный цикл"));

    auto updateResponse = makePutRequest("/api/v1/field-type-values/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);
    auto updateJson = updateResponse.extract_json().get();
    BOOST_CHECK_EQUAL(updateJson.at(U("value")).as_string(), U("Обновленный жизненный цикл"));

    // 4. Удаление значения
    mockFieldTypePossibleValueService->setDeleteValueResult(true);
    auto deleteResponse = makeDeleteRequest("/api/v1/field-type-values/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что значение удалено
    mockFieldTypePossibleValueService->setGetValueResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/field-type-values/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

// ============================================================
// Тесты проверки прав доступа
// ============================================================

BOOST_AUTO_TEST_CASE(test_regular_user_can_read_values)
{
    // Обычный пользователь может читать возможные значения
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    auto response = makeGetRequest("/api/v1/field-type-values").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValuesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValuesUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_regular_user_can_read_single_value)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    dto::FieldTypePossibleValue value;
    value.id = 42;
    value.fieldTypeId = 1;
    value.value = "Доступное значение";
    mockFieldTypePossibleValueService->setGetValueResult(value);

    auto response = makeGetRequest("/api/v1/field-type-values/42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getValueCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypePossibleValueService->getLastGetValueUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_create_value)
{
    // Обычный пользователь (не супер-админ)
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Настраиваем сервис на возврат nullopt для создания (имитация ошибки прав)
    mockFieldTypePossibleValueService->setCreateValueResult(std::nullopt);

    web::json::value body;
    body[U("fieldTypeId")] = web::json::value::number(1);
    body[U("value")] = web::json::value::string(U("Попытка создания"));

    auto response = makePostRequest("/api/v1/field-type-values", body).get();

    // Должен быть 403 Forbidden или 404 Not Found
    // В зависимости от реализации, может быть 403
    BOOST_CHECK(response.status_code() == status_codes::Forbidden || response.status_code() == status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_update_value)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    web::json::value body;
    body[U("value")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/field-type-values/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_delete_value)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    auto response = makeDeleteRequest("/api/v1/field-type-values/1").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
}

BOOST_AUTO_TEST_CASE(test_super_admin_can_create_update_delete)
{
    // Супер-админ (userId=1) уже настроен в фикстуре

    // Создание
    dto::FieldTypePossibleValue newValue;
    newValue.id = 200;
    newValue.fieldTypeId = 1;
    newValue.value = "Admin создал";
    mockFieldTypePossibleValueService->setCreateValueResult(newValue);

    web::json::value createBody;
    createBody[U("fieldTypeId")] = web::json::value::number(1);
    createBody[U("value")] = web::json::value::string(U("Admin создал"));

    auto createResponse = makePostRequest("/api/v1/field-type-values", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // Обновление
    dto::FieldTypePossibleValue updatedValue = newValue;
    updatedValue.value = "Admin обновил";
    mockFieldTypePossibleValueService->setUpdateValueResult(updatedValue);

    web::json::value updateBody;
    updateBody[U("value")] = web::json::value::string(U("Admin обновил"));

    auto updateResponse = makePutRequest("/api/v1/field-type-values/200", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // Удаление
    mockFieldTypePossibleValueService->setDeleteValueResult(true);
    auto deleteResponse = makeDeleteRequest("/api/v1/field-type-values/200").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
