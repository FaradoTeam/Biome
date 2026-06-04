#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_role_menu_item_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct RoleMenuItemsTestFixture
{
    RoleMenuItemsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockRoleMenuItemService = std::make_shared<MockRoleMenuItemService>();

        // Используем супер-админа (userId=1) для создания/обновления/удаления
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        server = std::make_unique<RestServer>("127.0.0.1", 18096);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setRoleMenuItemService(mockRoleMenuItemService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            { server->start(); }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    ~RoleMenuItemsTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18096"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18096"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18096"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18096"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockRoleMenuItemService> mockRoleMenuItemService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(RoleMenuItemsCrudTestSuite, RoleMenuItemsTestFixture)

// ============================================================
// GET /api/role-menu-items — Получение списка пунктов меню
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_role_menu_items_returns_list)
{
    services::RoleMenuItemsPage testPage;
    dto::RoleMenuItem item1;
    item1.id = 1;
    item1.roleId = 10;
    item1.caption = "Dashboard";
    item1.link = "/dashboard";
    dto::RoleMenuItem item2;
    item2.id = 2;
    item2.roleId = 10;
    item2.caption = "Settings";
    item2.link = "/settings";
    testPage.items = { item1, item2 };
    testPage.totalCount = 2;
    mockRoleMenuItemService->setGetRoleMenuItemsResult(testPage);

    auto response = makeGetRequest("/api/role-menu-items").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getGetRoleMenuItemsCallCount(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_role_menu_items_with_pagination)
{
    services::RoleMenuItemsPage emptyPage;
    mockRoleMenuItemService->setGetRoleMenuItemsResult(emptyPage);

    auto response = makeGetRequest("/api/role-menu-items?page=2&pageSize=15").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getLastGetRoleMenuItemsPage(), 2);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getLastGetRoleMenuItemsPageSize(), 15);
}

BOOST_AUTO_TEST_CASE(test_get_role_menu_items_with_role_filter)
{
    services::RoleMenuItemsPage emptyPage;
    mockRoleMenuItemService->setGetRoleMenuItemsResult(emptyPage);

    auto response = makeGetRequest("/api/role-menu-items?roleId=42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockRoleMenuItemService->getLastGetRoleMenuItemsRoleId().has_value());
    BOOST_CHECK_EQUAL(*mockRoleMenuItemService->getLastGetRoleMenuItemsRoleId(), 42);
}

// ============================================================
// GET /api/role-menu-items/{id} — Получение пункта меню по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_role_menu_item_by_id_success)
{
    dto::RoleMenuItem item;
    item.id = 5;
    item.roleId = 10;
    item.caption = "Profile";
    item.link = "/profile";
    mockRoleMenuItemService->setGetRoleMenuItemResult(item);

    auto response = makeGetRequest("/api/role-menu-items/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getLastGetRoleMenuItemId(), 5);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Profile"));
    BOOST_CHECK_EQUAL(json.at(U("link")).as_string(), U("/profile"));
}

BOOST_AUTO_TEST_CASE(test_get_role_menu_item_not_found)
{
    mockRoleMenuItemService->setGetRoleMenuItemResult(std::nullopt);

    auto response = makeGetRequest("/api/role-menu-items/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/role-menu-items — Создание пункта меню
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_role_menu_item_success)
{
    dto::RoleMenuItem created;
    created.id = 10;
    created.roleId = 5;
    created.caption = "New Item";
    created.link = "/new";
    mockRoleMenuItemService->setCreateRoleMenuItemResult(created);

    json::value body;
    body[U("roleId")] = json::value::number(5);
    body[U("caption")] = json::value::string(U("New Item"));
    body[U("link")] = json::value::string(U("/new"));
    body[U("icon")] = json::value::string(U("icon.png"));

    auto response = makePostRequest("/api/role-menu-items", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getCreateRoleMenuItemCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRoleMenuItemService->getLastCreatedRoleMenuItem().roleId, 5);
    BOOST_CHECK_EQUAL(*mockRoleMenuItemService->getLastCreatedRoleMenuItem().caption, "New Item");
}

BOOST_AUTO_TEST_CASE(test_create_role_menu_item_missing_required_fields)
{
    mockRoleMenuItemService->setCreateRoleMenuItemResult(std::nullopt);

    json::value body;
    body[U("roleId")] = json::value::number(5);
    body[U("link")] = json::value::string(U("/new"));

    auto response = makePostRequest("/api/role-menu-items", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    // Сервис НЕ должен быть вызван, так как валидация происходит в handler
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getCreateRoleMenuItemCallCount(), 0);
}

// ============================================================
// PUT /api/role-menu-items/{id} — Обновление пункта меню
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_role_menu_item_success)
{
    dto::RoleMenuItem updated;
    updated.id = 1;
    updated.caption = "Updated Caption";
    mockRoleMenuItemService->setUpdateRoleMenuItemResult(updated);

    json::value body;
    body[U("caption")] = json::value::string(U("Updated Caption"));

    auto response = makePutRequest("/api/role-menu-items/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getUpdateRoleMenuItemCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockRoleMenuItemService->getLastUpdatedRoleMenuItem().id, 1);
}

BOOST_AUTO_TEST_CASE(test_update_role_menu_item_not_found)
{
    mockRoleMenuItemService->setUpdateRoleMenuItemResult(std::nullopt);

    json::value body;
    body[U("caption")] = json::value::string(U("Ghost"));

    auto response = makePutRequest("/api/role-menu-items/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/role-menu-items/{id} — Удаление пункта меню
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_role_menu_item_success)
{
    mockRoleMenuItemService->setDeleteRoleMenuItemResult(true);

    auto response = makeDeleteRequest("/api/role-menu-items/3").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getDeleteRoleMenuItemCallCount(), 1);
    BOOST_CHECK_EQUAL(mockRoleMenuItemService->getLastDeletedRoleMenuItemId(), 3);
}

BOOST_AUTO_TEST_CASE(test_delete_role_menu_item_not_found)
{
    mockRoleMenuItemService->setDeleteRoleMenuItemResult(false);

    auto response = makeDeleteRequest("/api/role-menu-items/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
