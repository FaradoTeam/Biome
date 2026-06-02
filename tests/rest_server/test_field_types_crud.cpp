#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_field_type_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct FieldTypesTestFixture
{
    FieldTypesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockFieldTypeService = std::make_shared<MockFieldTypeService>();

        // Супер-админ (userId=1) для создания/обновления/удаления типов полей
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultFieldTypeService();

        server = std::make_unique<RestServer>("127.0.0.1", 18087);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setFieldTypeService(mockFieldTypeService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    void setupDefaultFieldTypeService()
    {
        // Настройка списка типов полей
        services::FieldTypesPage testPage;

        dto::FieldType stringField;
        stringField.id = 1;
        stringField.caption = "Заголовок";
        stringField.valueType = "String";
        stringField.itemTypeId = 1;
        stringField.isBoardVisible = true;
        stringField.description = "Заголовок элемента";

        dto::FieldType selectField;
        selectField.id = 2;
        selectField.caption = "Приоритет";
        selectField.valueType = "Select";
        selectField.itemTypeId = 1;
        selectField.isBoardVisible = true;
        selectField.description = "Приоритет задачи";

        dto::FieldType markdownField;
        markdownField.id = 3;
        markdownField.caption = "Описание";
        markdownField.valueType = "MarkdownText";
        markdownField.itemTypeId = 1;
        markdownField.isBoardVisible = false;
        markdownField.description = "Подробное описание";

        testPage.fieldTypes = { stringField, selectField, markdownField };
        testPage.totalCount = 3;
        mockFieldTypeService->setGetFieldTypesResult(testPage);
        mockFieldTypeService->setGetFieldTypeResult(stringField);

        dto::FieldType newField;
        newField.id = 4;
        newField.caption = "Сложность";
        newField.valueType = "Integer";
        newField.itemTypeId = 1;
        newField.isBoardVisible = true;
        mockFieldTypeService->setCreateFieldTypeResult(newField);

        dto::FieldType updatedField = stringField;
        updatedField.caption = "Заголовок (обновленный)";
        updatedField.isBoardVisible = false;
        mockFieldTypeService->setUpdateFieldTypeResult(updatedField);
        mockFieldTypeService->setDeleteFieldTypeResult(true);

        std::vector<dto::FieldType> itemTypeFields = { stringField, selectField };
        mockFieldTypeService->setFieldTypesByItemTypeResult(itemTypeFields);
    }

    ~FieldTypesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18087"));
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
    std::shared_ptr<MockFieldTypeService> mockFieldTypeService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(FieldTypesCrudTestSuite, FieldTypesTestFixture)

// ============================================================
// GET /api/field-types — Получение списка типов полей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_field_types_returns_list)
{
    auto response = makeGetRequest("/api/field-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getFieldTypesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_field_types_with_pagination_params)
{
    services::FieldTypesPage emptyPage;
    mockFieldTypeService->setGetFieldTypesResult(emptyPage);

    auto response = makeGetRequest("/api/field-types?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastGetFieldTypesPage(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_field_types_with_item_type_filter)
{
    services::FieldTypesPage filteredPage;
    dto::FieldType fieldType;
    fieldType.id = 10;
    fieldType.caption = "FilteredField";
    fieldType.valueType = "String";
    fieldType.itemTypeId = 42;
    filteredPage.fieldTypes = { fieldType };
    filteredPage.totalCount = 1;
    mockFieldTypeService->setGetFieldTypesResult(filteredPage);

    auto response = makeGetRequest("/api/field-types?itemTypeId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_field_types_with_value_type_filter)
{
    services::FieldTypesPage filteredPage;
    dto::FieldType fieldType;
    fieldType.id = 20;
    fieldType.caption = "SelectField";
    fieldType.valueType = "Select";
    filteredPage.fieldTypes = { fieldType };
    filteredPage.totalCount = 1;
    mockFieldTypeService->setGetFieldTypesResult(filteredPage);

    auto response = makeGetRequest("/api/field-types?valueType=Select").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_field_types_empty_list)
{
    services::FieldTypesPage emptyPage;
    mockFieldTypeService->setGetFieldTypesResult(emptyPage);

    auto response = makeGetRequest("/api/field-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_field_types_requires_auth)
{
    auto response = makeGetRequest("/api/field-types", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getFieldTypesCallCount(), 0);
}

// ============================================================
// GET /api/field-types/{id} — Получение типа поля по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_field_type_by_id_success)
{
    dto::FieldType fieldType;
    fieldType.id = 42;
    fieldType.caption = "Конкретное поле";
    fieldType.valueType = "Integer";
    fieldType.itemTypeId = 1;
    fieldType.isBoardVisible = true;
    mockFieldTypeService->setGetFieldTypeResult(fieldType);

    auto response = makeGetRequest("/api/field-types/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastGetFieldTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретное поле"));
    BOOST_CHECK_EQUAL(json.at(U("valueType")).as_string(), U("Integer"));
    BOOST_CHECK_EQUAL(json.at(U("isBoardVisible")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_field_type_not_found)
{
    mockFieldTypeService->setGetFieldTypeResult(std::nullopt);

    auto response = makeGetRequest("/api/field-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastGetFieldTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_field_type_invalid_id)
{
    auto response = makeGetRequest("/api/field-types/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/field-types — Создание типа поля
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_field_type_success)
{
    dto::FieldType createdField;
    createdField.id = 100;
    createdField.caption = "Новое поле";
    createdField.valueType = "String";
    createdField.itemTypeId = 1;
    createdField.isBoardVisible = true;
    createdField.description = "Описание поля";
    mockFieldTypeService->setCreateFieldTypeResult(createdField);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новое поле"));
    body[U("valueType")] = web::json::value::string(U("String"));
    body[U("itemTypeId")] = web::json::value::number(1);
    body[U("isBoardVisible")] = web::json::value::boolean(true);
    body[U("description")] = web::json::value::string(U("Описание поля"));

    auto response = makePostRequest("/api/field-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockFieldTypeService->createFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockFieldTypeService->getLastCreatedFieldType().caption,
        "Новое поле"
    );
    BOOST_CHECK_EQUAL(
        *mockFieldTypeService->getLastCreatedFieldType().valueType,
        "String"
    );
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastCreateFieldTypeUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новое поле"));
    BOOST_CHECK_EQUAL(json.at(U("valueType")).as_string(), U("String"));
}

BOOST_AUTO_TEST_CASE(test_create_field_type_missing_required_fields)
{
    mockFieldTypeService->setCreateFieldTypeResult(std::nullopt);

    web::json::value body;
    body[U("valueType")] = web::json::value::string(U("String"));

    auto response = makePostRequest("/api/field-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockFieldTypeService->createFieldTypeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_field_type_with_all_value_types)
{
    std::vector<std::string> valueTypes = {
        "String", "MarkdownText", "Integer", "Float", "Bool",
        "DateTime", "Date", "Uri", "Select", "SelectMulti",
        "ItemId", "UserId", "ProjectId", "PhaseId",
        "ItemsIds", "UsersIds", "ProjectsIds", "PhasesIds"
    };

    for (size_t i = 0; i < valueTypes.size(); ++i)
    {
        dto::FieldType createdField;
        createdField.id = static_cast<int64_t>(200 + i);
        createdField.caption = "Field " + valueTypes[i];
        createdField.valueType = valueTypes[i];
        createdField.itemTypeId = 1;
        mockFieldTypeService->setCreateFieldTypeResult(createdField);

        web::json::value body;
        body[U("caption")] = web::json::value::string(
            U("Field " + valueTypes[i])
        );
        body[U("valueType")] = web::json::value::string(
            U(valueTypes[i])
        );
        body[U("itemTypeId")] = web::json::value::number(1);

        auto response = makePostRequest("/api/field-types", body).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    }
}

BOOST_AUTO_TEST_CASE(test_create_field_type_not_board_visible)
{
    dto::FieldType createdField;
    createdField.id = 300;
    createdField.caption = "Скрытое поле";
    createdField.valueType = "String";
    createdField.itemTypeId = 1;
    createdField.isBoardVisible = false;
    mockFieldTypeService->setCreateFieldTypeResult(createdField);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Скрытое поле"));
    body[U("valueType")] = web::json::value::string(U("String"));
    body[U("itemTypeId")] = web::json::value::number(1);
    body[U("isBoardVisible")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/field-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("isBoardVisible")).as_bool(), false);
}

// ============================================================
// PUT /api/field-types/{id} — Обновление типа поля
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_field_type_success)
{
    dto::FieldType updatedField;
    updatedField.id = 1;
    updatedField.caption = "Обновленное поле";
    updatedField.valueType = "MarkdownText";
    updatedField.isBoardVisible = false;
    mockFieldTypeService->setUpdateFieldTypeResult(updatedField);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновленное поле"));
    body[U("valueType")] = web::json::value::string(U("MarkdownText"));
    body[U("isBoardVisible")] = web::json::value::boolean(false);

    auto response = makePutRequest("/api/field-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypeService->updateFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockFieldTypeService->getLastUpdatedFieldType().id,
        1
    );
    BOOST_CHECK_EQUAL(
        *mockFieldTypeService->getLastUpdatedFieldType().caption,
        "Обновленное поле"
    );
    BOOST_CHECK_EQUAL(
        *mockFieldTypeService->getLastUpdatedFieldType().valueType,
        "MarkdownText"
    );
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastUpdateFieldTypeUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновленное поле"));
    BOOST_CHECK_EQUAL(json.at(U("isBoardVisible")).as_bool(), false);
}

BOOST_AUTO_TEST_CASE(test_update_field_type_partial)
{
    dto::FieldType updatedField;
    updatedField.id = 1;
    updatedField.caption = "Только название";
    updatedField.valueType = "String";
    mockFieldTypeService->setUpdateFieldTypeResult(updatedField);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Только название"));

    auto response = makePutRequest("/api/field-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockFieldTypeService->updateFieldTypeCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_field_type_not_found)
{
    mockFieldTypeService->setUpdateFieldTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующее"));

    auto response = makePutRequest("/api/field-types/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockFieldTypeService->updateFieldTypeCallCount(), 1);
}

// ============================================================
// DELETE /api/field-types/{id} — Удаление типа поля
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_field_type_success)
{
    mockFieldTypeService->setDeleteFieldTypeResult(true);

    auto response = makeDeleteRequest("/api/field-types/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockFieldTypeService->deleteFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastDeletedFieldTypeId(), 3);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastDeleteFieldTypeUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_field_type_not_found)
{
    mockFieldTypeService->setDeleteFieldTypeResult(false);

    auto response = makeDeleteRequest("/api/field-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockFieldTypeService->deleteFieldTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockFieldTypeService->getLastDeletedFieldTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_field_type_requires_auth)
{
    auto response = makeDeleteRequest("/api/field-types/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockFieldTypeService->deleteFieldTypeCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_field_type_lifecycle)
{
    // 1. Создание типа поля
    dto::FieldType newField;
    newField.id = 100;
    newField.caption = "Жизненный цикл поля";
    newField.valueType = "Select";
    newField.itemTypeId = 1;
    newField.isBoardVisible = true;
    mockFieldTypeService->setCreateFieldTypeResult(newField);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл поля"));
    createBody[U("valueType")] = web::json::value::string(U("Select"));
    createBody[U("itemTypeId")] = web::json::value::number(1);
    createBody[U("isBoardVisible")] = web::json::value::boolean(true);

    auto createResponse = makePostRequest("/api/field-types", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданного типа поля
    mockFieldTypeService->setGetFieldTypeResult(newField);
    auto getResponse = makeGetRequest("/api/field-types/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление типа поля
    dto::FieldType updatedField = newField;
    updatedField.caption = "Обновленное поле";
    updatedField.valueType = "Integer";
    updatedField.isBoardVisible = false;
    mockFieldTypeService->setUpdateFieldTypeResult(updatedField);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновленное поле"));
    updateBody[U("valueType")] = web::json::value::string(U("Integer"));
    updateBody[U("isBoardVisible")] = web::json::value::boolean(false);

    auto updateResponse = makePutRequest("/api/field-types/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление типа поля
    mockFieldTypeService->setDeleteFieldTypeResult(true);
    auto deleteResponse = makeDeleteRequest("/api/field-types/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что тип поля удален
    mockFieldTypeService->setGetFieldTypeResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/field-types/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
