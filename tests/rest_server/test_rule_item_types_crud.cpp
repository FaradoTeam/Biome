#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_rule_item_type_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RuleItemTypesTestFixture
{
    RuleItemTypesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRuleItemTypeService = std::make_shared<MockRuleItemTypeService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18094);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRuleItemTypeService(mockRuleItemTypeService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    ~RuleItemTypesTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18094"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18094"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18094"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18094"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRuleItemTypeService> mockRuleItemTypeService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RuleItemTypesCrudTestSuite, RuleItemTypesTestFixture)

// ============================================================
// GET /api/rule-item-types — Получение списка прав на типы элементов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_item_types_returns_list)
{
    services::RuleItemTypesPage testPage;
    dto::RuleItemType rit1;
    rit1.id = 1;
    rit1.ruleId = 10;
    rit1.itemTypeId = 100;
    rit1.isReader = true;
    dto::RuleItemType rit2;
    rit2.id = 2;
    rit2.ruleId = 20;
    rit2.itemTypeId = 200;
    rit2.isWriter = true;
    testPage.items = { rit1, rit2 };
    testPage.totalCount = 2;
    mockRuleItemTypeService->setGetRuleItemTypesResult(testPage);

    auto response = makeGetRequest("/api/rule-item-types").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getGetRuleItemTypesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_rule_item_types_with_pagination)
{
    services::RuleItemTypesPage emptyPage;
    mockRuleItemTypeService->setGetRuleItemTypesResult(emptyPage);

    auto response = makeGetRequest("/api/rule-item-types?page=2&pageSize=15").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getLastGetRuleItemTypesPage(), 2);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getLastGetRuleItemTypesPageSize(), 15);
}

BOOST_AUTO_TEST_CASE(test_get_rule_item_types_with_rule_filter)
{
    services::RuleItemTypesPage emptyPage;
    mockRuleItemTypeService->setGetRuleItemTypesResult(emptyPage);

    auto response = makeGetRequest("/api/rule-item-types?ruleId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleItemTypeService->getLastGetRuleItemTypesRuleId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleItemTypeService->getLastGetRuleItemTypesRuleId(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_rule_item_types_with_item_type_filter)
{
    services::RuleItemTypesPage emptyPage;
    mockRuleItemTypeService->setGetRuleItemTypesResult(emptyPage);

    auto response = makeGetRequest("/api/rule-item-types?itemTypeId=100").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRuleItemTypeService->getLastGetRuleItemTypesItemTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockRuleItemTypeService->getLastGetRuleItemTypesItemTypeId(), 100);
}

// ============================================================
// GET /api/rule-item-types/{id} — Получение права по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_rule_item_type_by_id_success)
{
    dto::RuleItemType rit;
    rit.id = 5;
    rit.ruleId = 10;
    rit.itemTypeId = 100;
    mockRuleItemTypeService->setGetRuleItemTypeResult(rit);

    auto response = makeGetRequest("/api/rule-item-types/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getLastGetRuleItemTypeId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("ruleId")).as_integer(), 10);
    BOOST_CHECK_EQUAL(json.at(U("itemTypeId")).as_integer(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_rule_item_type_not_found)
{
    mockRuleItemTypeService->setGetRuleItemTypeResult(std::nullopt);

    auto response = makeGetRequest("/api/rule-item-types/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/rule-item-types — Создание права на тип элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_rule_item_type_success)
{
    dto::RuleItemType created;
    created.id = 10;
    created.ruleId = 5;
    created.itemTypeId = 50;
    mockRuleItemTypeService->setCreateRuleItemTypeResult(created);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("itemTypeId")] = json::value::number(50);
    body[U("isReader")] = json::value::boolean(true);

    auto response = makePostRequest("/api/rule-item-types", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getCreateRuleItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleItemTypeService->getLastCreatedRuleItemType().ruleId, 5);
    BOOST_CHECK_EQUAL(*mockRuleItemTypeService->getLastCreatedRuleItemType().itemTypeId, 50);
}

BOOST_AUTO_TEST_CASE(test_create_rule_item_type_missing_required_fields)
{
    mockRuleItemTypeService->setCreateRuleItemTypeResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);

    auto response = makePostRequest("/api/rule-item-types", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getCreateRuleItemTypeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_rule_item_type_duplicate)
{
    mockRuleItemTypeService->setCreateRuleItemTypeResult(std::nullopt);

    json::value body;
    body[U("ruleId")] = json::value::number(5);
    body[U("itemTypeId")] = json::value::number(50);

    auto response = makePostRequest("/api/rule-item-types", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden); // TODO: status_codes::Conflict
}

// ============================================================
// PUT /api/rule-item-types/{id} — Обновление права на тип элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_rule_item_type_success)
{
    dto::RuleItemType updated;
    updated.id = 1;
    updated.isReader = true;
    updated.isWriter = false;
    mockRuleItemTypeService->setUpdateRuleItemTypeResult(updated);

    json::value body;
    body[U("isReader")] = json::value::boolean(true);
    body[U("isWriter")] = json::value::boolean(false);

    auto response = makePutRequest("/api/rule-item-types/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getUpdateRuleItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRuleItemTypeService->getLastUpdatedRuleItemType().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_rule_item_type_not_found)
{
    mockRuleItemTypeService->setUpdateRuleItemTypeResult(std::nullopt);

    json::value body;
    body[U("isReader")] = json::value::boolean(true);

    auto response = makePutRequest("/api/rule-item-types/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/rule-item-types/{id} — Удаление права на тип элемента
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_rule_item_type_success)
{
    mockRuleItemTypeService->setDeleteRuleItemTypeResult(true);

    auto response = makeDeleteRequest("/api/rule-item-types/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getDeleteRuleItemTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getLastDeletedRuleItemTypeId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_item_type_not_found)
{
    mockRuleItemTypeService->setDeleteRuleItemTypeResult(false);

    auto response = makeDeleteRequest("/api/rule-item-types/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_rule_item_type_requires_auth)
{
    auto response = makeDeleteRequest("/api/rule-item-types/1", "").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockRuleItemTypeService->getDeleteRuleItemTypeCallCount(), 0);
}

// ============================================================
// Интеграционный тест
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_rule_item_type_lifecycle)
{
    // 1. Создание
    dto::RuleItemType newRit;
    newRit.id = 100;
    newRit.ruleId = 50;
    newRit.itemTypeId = 500;
    mockRuleItemTypeService->setCreateRuleItemTypeResult(newRit);

    json::value createBody;
    createBody[U("ruleId")] = json::value::number(50);
    createBody[U("itemTypeId")] = json::value::number(500);

    auto createResponse = makePostRequest("/api/rule-item-types", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение
    mockRuleItemTypeService->setGetRuleItemTypeResult(newRit);
    auto getResponse = makeGetRequest("/api/rule-item-types/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    dto::RuleItemType updatedRit = newRit;
    updatedRit.isWriter = true;
    mockRuleItemTypeService->setUpdateRuleItemTypeResult(updatedRit);

    json::value updateBody;
    updateBody[U("isWriter")] = json::value::boolean(true);

    auto updateResponse = makePutRequest("/api/rule-item-types/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление
    mockRuleItemTypeService->setDeleteRuleItemTypeResult(true);
    auto deleteResponse = makeDeleteRequest("/api/rule-item-types/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
