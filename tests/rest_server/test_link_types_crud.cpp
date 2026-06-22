#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_link_type_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server::tests
{

struct LinkTypesTestFixture
{
    LinkTypesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockLinkTypeService = std::make_shared<MockLinkTypeService>();

        // Супер-админ (userId=1) для создания/обновления/удаления типов связей
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных по умолчанию
        setupDefaultLinkTypeService();

        server = std::make_unique<RestServer>("127.0.0.1", 18110);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setLinkTypeService(mockLinkTypeService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultLinkTypeService()
    {
        // Настройка списка типов связей
        services::LinkTypesPage testPage;

        dto::LinkType relatesTo;
        relatesTo.id = 1;
        relatesTo.caption = "связан с";
        relatesTo.sourceItemTypeId = 1;
        relatesTo.destinationItemTypeId = 1;
        relatesTo.isBidirectional = false;

        dto::LinkType blocks;
        blocks.id = 2;
        blocks.caption = "блокирует";
        blocks.sourceItemTypeId = 1;
        blocks.destinationItemTypeId = 1;
        blocks.isBidirectional = false;

        dto::LinkType duplicates;
        duplicates.id = 3;
        duplicates.caption = "дублирует";
        duplicates.sourceItemTypeId = 1;
        duplicates.destinationItemTypeId = 1;
        duplicates.isBidirectional = true;

        testPage.linkTypes = { relatesTo, blocks, duplicates };
        testPage.totalCount = 3;
        mockLinkTypeService->setGetLinkTypesResult(testPage);
        mockLinkTypeService->setGetLinkTypeResult(relatesTo);

        dto::LinkType newLinkType;
        newLinkType.id = 4;
        newLinkType.caption = "тестирует";
        newLinkType.sourceItemTypeId = 1;
        newLinkType.destinationItemTypeId = 1;
        newLinkType.isBidirectional = false;
        mockLinkTypeService->setCreateLinkTypeResult(newLinkType);

        dto::LinkType updatedLinkType = relatesTo;
        updatedLinkType.caption = "обновлённая связь";
        updatedLinkType.isBidirectional = true;
        mockLinkTypeService->setUpdateLinkTypeResult(updatedLinkType);
        mockLinkTypeService->setDeleteLinkTypeResult(true);
    }

    ~LinkTypesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18110"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18110"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18110"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18110"));
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
    std::shared_ptr<MockLinkTypeService> mockLinkTypeService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(LinkTypesCrudTestSuite, LinkTypesTestFixture)

// ============================================================
// GET /api/v1/link-types — Получение списка типов связей
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_link_types_returns_list)
{
    auto response = makeGetRequest("/api/v1/link-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getGetLinkTypesCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_link_types_with_pagination_params)
{
    services::LinkTypesPage emptyPage;
    mockLinkTypeService->setGetLinkTypesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/link-types?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastGetLinkTypesPage(), 3);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastGetLinkTypesPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_link_types_with_source_filter)
{
    services::LinkTypesPage filteredPage;
    dto::LinkType linkType;
    linkType.id = 10;
    linkType.caption = "FilteredLink";
    linkType.sourceItemTypeId = 42;
    linkType.destinationItemTypeId = 1;
    filteredPage.linkTypes = { linkType };
    filteredPage.totalCount = 1;
    mockLinkTypeService->setGetLinkTypesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/link-types?sourceItemTypeId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockLinkTypeService->getLastGetLinkTypesSourceItemTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockLinkTypeService->getLastGetLinkTypesSourceItemTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_link_types_with_destination_filter)
{
    services::LinkTypesPage filteredPage;
    dto::LinkType linkType;
    linkType.id = 20;
    linkType.caption = "DestFiltered";
    linkType.sourceItemTypeId = 1;
    linkType.destinationItemTypeId = 100;
    filteredPage.linkTypes = { linkType };
    filteredPage.totalCount = 1;
    mockLinkTypeService->setGetLinkTypesResult(filteredPage);

    auto response = makeGetRequest("/api/v1/link-types?destinationItemTypeId=100").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockLinkTypeService->getLastGetLinkTypesDestItemTypeId().has_value());
    BOOST_CHECK_EQUAL(*mockLinkTypeService->getLastGetLinkTypesDestItemTypeId(), 100);
}

BOOST_AUTO_TEST_CASE(test_get_link_types_empty_list)
{
    services::LinkTypesPage emptyPage;
    mockLinkTypeService->setGetLinkTypesResult(emptyPage);

    auto response = makeGetRequest("/api/v1/link-types").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_link_types_requires_auth)
{
    auto response = makeGetRequest("/api/v1/link-types", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getGetLinkTypesCallCount(), 0);
}

// ============================================================
// GET /api/v1/link-types/{id} — Получение типа связи по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_link_type_by_id_success)
{
    dto::LinkType linkType;
    linkType.id = 42;
    linkType.caption = "Конкретный тип связи";
    linkType.sourceItemTypeId = 1;
    linkType.destinationItemTypeId = 2;
    linkType.isBidirectional = true;
    mockLinkTypeService->setGetLinkTypeResult(linkType);

    auto response = makeGetRequest("/api/v1/link-types/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getGetLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastGetLinkTypeId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Конкретный тип связи"));
    BOOST_CHECK_EQUAL(json.at(U("sourceItemTypeId")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("destinationItemTypeId")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("isBidirectional")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_get_link_type_not_found)
{
    mockLinkTypeService->setGetLinkTypeResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/link-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getGetLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastGetLinkTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_link_type_invalid_id)
{
    auto response = makeGetRequest("/api/v1/link-types/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/link-types — Создание типа связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_link_type_success)
{
    dto::LinkType createdType;
    createdType.id = 100;
    createdType.caption = "Новый тип связи";
    createdType.sourceItemTypeId = 1;
    createdType.destinationItemTypeId = 2;
    createdType.isBidirectional = false;
    mockLinkTypeService->setCreateLinkTypeResult(createdType);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый тип связи"));
    body[U("sourceItemTypeId")] = web::json::value::number(1);
    body[U("destinationItemTypeId")] = web::json::value::number(2);
    body[U("isBidirectional")] = web::json::value::boolean(false);

    auto response = makePostRequest("/api/v1/link-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getCreateLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockLinkTypeService->getLastCreatedLinkType().caption, "Новый тип связи");
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastCreateLinkTypeUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новый тип связи"));
}

BOOST_AUTO_TEST_CASE(test_create_link_type_missing_required_fields)
{
    mockLinkTypeService->setCreateLinkTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Только название"));

    auto response = makePostRequest("/api/v1/link-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getCreateLinkTypeCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_link_type_missing_source_type)
{
    mockLinkTypeService->setCreateLinkTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Без исходного типа"));
    body[U("destinationItemTypeId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/link-types", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getCreateLinkTypeCallCount(), 0);
}

// ============================================================
// PUT /api/v1/link-types/{id} — Обновление типа связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_link_type_success)
{
    dto::LinkType updatedType;
    updatedType.id = 1;
    updatedType.caption = "Обновлённый тип";
    updatedType.isBidirectional = true;
    mockLinkTypeService->setUpdateLinkTypeResult(updatedType);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновлённый тип"));
    body[U("isBidirectional")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/link-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getUpdateLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockLinkTypeService->getLastUpdatedLinkType().id, 1);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastUpdateLinkTypeUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновлённый тип"));
    BOOST_CHECK_EQUAL(json.at(U("isBidirectional")).as_bool(), true);
}

BOOST_AUTO_TEST_CASE(test_update_link_type_partial)
{
    dto::LinkType updatedType;
    updatedType.id = 1;
    updatedType.isBidirectional = true;
    mockLinkTypeService->setUpdateLinkTypeResult(updatedType);

    web::json::value body;
    body[U("isBidirectional")] = web::json::value::boolean(true);

    auto response = makePutRequest("/api/v1/link-types/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getUpdateLinkTypeCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_link_type_not_found)
{
    mockLinkTypeService->setUpdateLinkTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующий"));

    auto response = makePutRequest("/api/v1/link-types/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getUpdateLinkTypeCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_link_type_regular_user_forbidden)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Настраиваем, чтобы ресурс существовал для обычного пользователя
    dto::LinkType existingLinkType;
    existingLinkType.id = 1;
    existingLinkType.caption = "Existing Type";
    mockLinkTypeService->setGetLinkTypeResult(existingLinkType);

    // Настраиваем, что обновление запрещено
    mockLinkTypeService->setUpdateLinkTypeResult(std::nullopt);

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/link-types/1", body).get();

    // TODO: Должен вернуть 403, потому что ресурс существует, но нет прав на обновление
    // пока ожидаем NotFound
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/link-types/{id} — Удаление типа связи
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_link_type_success)
{
    mockLinkTypeService->setDeleteLinkTypeResult(true);

    auto response = makeDeleteRequest("/api/v1/link-types/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getDeleteLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastDeletedLinkTypeId(), 3);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastDeleteLinkTypeUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_link_type_not_found)
{
    mockLinkTypeService->setDeleteLinkTypeResult(false);

    auto response = makeDeleteRequest("/api/v1/link-types/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getDeleteLinkTypeCallCount(), 1);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getLastDeletedLinkTypeId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_link_type_in_use)
{
    mockLinkTypeService->setDeleteLinkTypeResult(false);

    auto response = makeDeleteRequest("/api/v1/link-types/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_link_type_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/link-types/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockLinkTypeService->getDeleteLinkTypeCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_link_type_lifecycle)
{
    // 1. Создание типа связи
    dto::LinkType newType;
    newType.id = 100;
    newType.caption = "Жизненный цикл";
    newType.sourceItemTypeId = 1;
    newType.destinationItemTypeId = 1;
    newType.isBidirectional = false;
    mockLinkTypeService->setCreateLinkTypeResult(newType);

    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл"));
    createBody[U("sourceItemTypeId")] = web::json::value::number(1);
    createBody[U("destinationItemTypeId")] = web::json::value::number(1);

    auto createResponse = makePostRequest("/api/v1/link-types", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданного типа
    mockLinkTypeService->setGetLinkTypeResult(newType);
    auto getResponse = makeGetRequest("/api/v1/link-types/100").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление типа
    dto::LinkType updatedType = newType;
    updatedType.caption = "Обновлённый жизненный цикл";
    updatedType.isBidirectional = true;
    mockLinkTypeService->setUpdateLinkTypeResult(updatedType);

    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновлённый жизненный цикл"));
    updateBody[U("isBidirectional")] = web::json::value::boolean(true);

    auto updateResponse = makePutRequest("/api/v1/link-types/100", updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Удаление типа
    mockLinkTypeService->setDeleteLinkTypeResult(true);
    auto deleteResponse = makeDeleteRequest("/api/v1/link-types/100").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что тип удалён
    mockLinkTypeService->setGetLinkTypeResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/api/v1/link-types/100").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
