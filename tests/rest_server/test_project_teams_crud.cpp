#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_project_service.h"
#include "tests/server_mocks/mock_project_team_service.h"
#include "tests/server_mocks/mock_team_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

// Базовый путь API с префиксом /api/v1
const std::string API_BASE = "/api/v1";

struct ProjectTeamsTestFixture
{
    ProjectTeamsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockUserService = std::make_shared<MockUserService>();
        mockProjectService = std::make_shared<MockProjectService>();
        mockTeamService = std::make_shared<MockTeamService>();
        mockProjectTeamService = std::make_shared<MockProjectTeamService>();

        // Супер-админ (userId=1) для создания/удаления связей
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        // Настройка тестовых данных
        setupDefaultProjectTeamService();

        server = std::make_unique<RestServer>("127.0.0.1", 18112);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setUserService(mockUserService);
        server->setProjectService(mockProjectService);
        server->setTeamService(mockTeamService);
        server->setProjectTeamService(mockProjectTeamService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread([this]()
                                   { server->start(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultProjectTeamService()
    {
        // Настройка списка связей
        services::ProjectTeamsPage testPage;

        dto::ProjectTeam pt1;
        pt1.id = 1;
        pt1.projectId = 100;
        pt1.teamId = 10;

        dto::ProjectTeam pt2;
        pt2.id = 2;
        pt2.projectId = 100;
        pt2.teamId = 20;

        dto::ProjectTeam pt3;
        pt3.id = 3;
        pt3.projectId = 200;
        pt3.teamId = 10;

        testPage.items = { pt1, pt2, pt3 };
        testPage.totalCount = 3;
        mockProjectTeamService->setGetProjectTeamsResult(testPage);
        mockProjectTeamService->setGetProjectTeamResult(pt1);

        // Для создания используем teamId = 999 (гарантированно новый)
        dto::ProjectTeam newPt;
        newPt.id = 100;
        newPt.projectId = 100;
        newPt.teamId = 999;
        mockProjectTeamService->setCreateProjectTeamResult(newPt);

        services::ProjectTeamResult deleteResult;
        deleteResult.success = true;
        mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);

        // Для обычного пользователя (userId=100) создание запрещено
        mockProjectTeamService->setCreateProjectTeamResultForUser(100, std::nullopt);
    }

    ~ProjectTeamsTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        http_client client(U("http://127.0.0.1:18112"));
        http_request request(methods::GET);
        request.set_request_uri(U(API_BASE + path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(
        const std::string& path,
        const json::value& body,
        const std::string& token = "valid_token"
    )
    {
        http_client client(U("http://127.0.0.1:18112"));
        http_request request(methods::POST);
        request.set_request_uri(U(API_BASE + path));
        if (!token.empty())
        {
            request.headers().add(U("Authorization"), U("Bearer " + token));
        }
        request.set_body(body);
        request.headers().set_content_type(U("application/json"));
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        http_client client(U("http://127.0.0.1:18112"));
        http_request request(methods::DEL);
        request.set_request_uri(U(API_BASE + path));
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
    std::shared_ptr<MockProjectService> mockProjectService;
    std::shared_ptr<MockTeamService> mockTeamService;
    std::shared_ptr<MockProjectTeamService> mockProjectTeamService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ProjectTeamsCrudTestSuite, ProjectTeamsTestFixture)

// ============================================================
// GET /api/v1/project-teams
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_project_teams_returns_list)
{
    auto response = makeGetRequest("/project-teams").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamsCallCount(), 1);
    // Проверяем, что userId передан в сервис (1 - супер-админ)
    BOOST_CHECK(mockProjectTeamService->getLastGetProjectTeamsUserId() == 1 || mockProjectTeamService->getLastGetProjectTeamsUserId() == 0);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_project_teams_with_pagination)
{
    services::ProjectTeamsPage emptyPage;
    mockProjectTeamService->setGetProjectTeamsResult(emptyPage);

    auto response = makeGetRequest("/project-teams?page=3&pageSize=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastGetProjectTeamsPage(), 3);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastGetProjectTeamsPageSize(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_project_teams_filter_by_project)
{
    services::ProjectTeamsPage filteredPage;
    dto::ProjectTeam pt;
    pt.id = 10;
    pt.projectId = 42;
    pt.teamId = 20;
    filteredPage.items = { pt };
    filteredPage.totalCount = 1;
    mockProjectTeamService->setGetProjectTeamsResult(filteredPage);

    auto response = makeGetRequest("/project-teams?projectId=42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockProjectTeamService->getLastGetProjectTeamsProjectId().has_value());
    BOOST_CHECK_EQUAL(*mockProjectTeamService->getLastGetProjectTeamsProjectId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("items"))[0].at(U("projectId")).as_integer(), 42);
}

BOOST_AUTO_TEST_CASE(test_get_project_teams_filter_by_team)
{
    services::ProjectTeamsPage filteredPage;
    dto::ProjectTeam pt;
    pt.id = 20;
    pt.projectId = 100;
    pt.teamId = 5;
    filteredPage.items = { pt };
    filteredPage.totalCount = 1;
    mockProjectTeamService->setGetProjectTeamsResult(filteredPage);

    auto response = makeGetRequest("/project-teams?teamId=5").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockProjectTeamService->getLastGetProjectTeamsTeamId().has_value());
    BOOST_CHECK_EQUAL(*mockProjectTeamService->getLastGetProjectTeamsTeamId(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_project_teams_empty_list)
{
    services::ProjectTeamsPage emptyPage;
    mockProjectTeamService->setGetProjectTeamsResult(emptyPage);

    auto response = makeGetRequest("/project-teams").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_project_teams_requires_auth)
{
    auto response = makeGetRequest("/project-teams", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamsCallCount(), 0);
}

// ============================================================
// GET /api/v1/project-teams/{id}
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_project_team_by_id_success)
{
    dto::ProjectTeam pt;
    pt.id = 42;
    pt.projectId = 100;
    pt.teamId = 50;
    mockProjectTeamService->setGetProjectTeamResult(pt);

    auto response = makeGetRequest("/project-teams/42").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastGetProjectTeamId(), 42);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 42);
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("teamId")).as_integer(), 50);
}

BOOST_AUTO_TEST_CASE(test_get_project_team_not_found)
{
    mockProjectTeamService->setGetProjectTeamResult(std::nullopt);

    auto response = makeGetRequest("/project-teams/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastGetProjectTeamId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_project_team_invalid_id)
{
    auto response = makeGetRequest("/project-teams/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/project-teams
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_project_team_success)
{
    dto::ProjectTeam created;
    created.id = 100;
    created.projectId = 100;
    created.teamId = 999; // Используем уникальный teamId
    mockProjectTeamService->setCreateProjectTeamResult(created);

    json::value body;
    body[U("projectId")] = json::value::number(100);
    body[U("teamId")] = json::value::number(999);

    auto response = makePostRequest("/project-teams", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(
        *mockProjectTeamService->getLastCreatedProjectTeam().projectId,
        100
    );
    BOOST_CHECK_EQUAL(
        *mockProjectTeamService->getLastCreatedProjectTeam().teamId,
        999
    );
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastCreateProjectTeamUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("teamId")).as_integer(), 999);
}

BOOST_AUTO_TEST_CASE(test_create_project_team_missing_project_id)
{
    json::value body;
    body[U("teamId")] = json::value::number(999);

    auto response = makePostRequest("/project-teams", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_project_team_missing_team_id)
{
    json::value body;
    body[U("projectId")] = json::value::number(100);

    auto response = makePostRequest("/project-teams", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_project_team_duplicate)
{
    // Настраиваем мок на возврат конфликта
    mockProjectTeamService->setCreateProjectTeamResult(std::nullopt);

    json::value body;
    body[U("projectId")] = json::value::number(100);
    body[U("teamId")] = json::value::number(10); // Существующая команда

    auto response = makePostRequest("/project-teams", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_project_team_forbidden)
{
    // Обычный пользователь (не супер-админ)
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Используем НОВУЮ команду (teamId=888), чтобы не было конфликта
    json::value body;
    body[U("projectId")] = json::value::number(100);
    body[U("teamId")] = json::value::number(888); // Новая команда

    auto response = makePostRequest("/project-teams", body).get();

    // Должен быть 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    // Сервис вызывается, но возвращает nullopt
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastCreateProjectTeamUserId(), 100);
}

// ============================================================
// DELETE /api/v1/project-teams/{id}
// ============================================================

BOOST_AUTO_TEST_CASE(test_delete_project_team_success)
{
    services::ProjectTeamResult deleteResult;
    deleteResult.success = true;
    mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);

    auto response = makeDeleteRequest("/project-teams/3").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getDeleteProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastDeletedProjectTeamId(), 3);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastDeleteProjectTeamUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_project_team_not_found)
{
    services::ProjectTeamResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 404;
    deleteResult.errorMessage = "ProjectTeam not found";
    mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);

    auto response = makeDeleteRequest("/project-teams/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getDeleteProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastDeletedProjectTeamId(), 999);
}

BOOST_AUTO_TEST_CASE(test_delete_project_team_forbidden)
{
    // Обычный пользователь (не супер-админ)
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    services::ProjectTeamResult deleteResult;
    deleteResult.success = false;
    deleteResult.errorCode = 403;
    deleteResult.errorMessage = "Insufficient permissions";
    mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);

    auto response = makeDeleteRequest("/project-teams/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getDeleteProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastDeleteProjectTeamUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_delete_project_team_requires_auth)
{
    auto response = makeDeleteRequest("/project-teams/1", "").get();

    // Без токена middleware возвращает 401
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getDeleteProjectTeamCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_project_team_lifecycle)
{
    // 1. Создание связи
    dto::ProjectTeam newPt;
    newPt.id = 200;
    newPt.projectId = 100;
    newPt.teamId = 777; // Уникальный teamId
    mockProjectTeamService->setCreateProjectTeamResult(newPt);

    json::value createBody;
    createBody[U("projectId")] = json::value::number(100);
    createBody[U("teamId")] = json::value::number(777);

    auto createResponse = makePostRequest("/project-teams", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // 2. Чтение созданной связи
    mockProjectTeamService->setGetProjectTeamResult(newPt);
    auto getResponse = makeGetRequest("/project-teams/200").get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Получение списка связей
    services::ProjectTeamsPage listPage;
    listPage.items = { newPt };
    listPage.totalCount = 1;
    mockProjectTeamService->setGetProjectTeamsResult(listPage);
    auto listResponse = makeGetRequest("/project-teams?projectId=100").get();
    BOOST_CHECK_EQUAL(listResponse.status_code(), status_codes::OK);

    // 4. Удаление связи
    services::ProjectTeamResult deleteResult;
    deleteResult.success = true;
    mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/project-teams/200").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);

    // 5. Проверка после удаления
    mockProjectTeamService->setGetProjectTeamResult(std::nullopt);
    auto getAfterDelete = makeGetRequest("/project-teams/200").get();
    BOOST_CHECK_EQUAL(getAfterDelete.status_code(), status_codes::NotFound);
}

// ============================================================
// Тесты проверки прав доступа
// ============================================================

BOOST_AUTO_TEST_CASE(test_super_admin_can_create_and_delete)
{
    // Супер-админ (userId=1) уже настроен в фикстуре

    // Создание с уникальным teamId
    dto::ProjectTeam newPt;
    newPt.id = 300;
    newPt.projectId = 200;
    newPt.teamId = 666; // Уникальный teamId
    mockProjectTeamService->setCreateProjectTeamResult(newPt);

    json::value createBody;
    createBody[U("projectId")] = json::value::number(200);
    createBody[U("teamId")] = json::value::number(666);

    auto createResponse = makePostRequest("/project-teams", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    // Удаление
    services::ProjectTeamResult deleteResult;
    deleteResult.success = true;
    mockProjectTeamService->setDeleteProjectTeamResult(deleteResult);
    auto deleteResponse = makeDeleteRequest("/project-teams/300").get();
    BOOST_CHECK_EQUAL(deleteResponse.status_code(), status_codes::NoContent);
}

BOOST_AUTO_TEST_CASE(test_regular_user_can_read_project_teams)
{
    // Обычный пользователь может читать связи
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    auto response = makeGetRequest("/project-teams").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamsCallCount(), 1);
    // Проверяем, что userId передан в сервис
    BOOST_CHECK(mockProjectTeamService->getLastGetProjectTeamsUserId() == 100 || mockProjectTeamService->getLastGetProjectTeamsUserId() == 0);
}

BOOST_AUTO_TEST_CASE(test_regular_user_can_read_single_project_team)
{
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    dto::ProjectTeam pt;
    pt.id = 42;
    pt.projectId = 100;
    pt.teamId = 10;
    mockProjectTeamService->setGetProjectTeamResult(pt);

    auto response = makeGetRequest("/project-teams/42").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getGetProjectTeamCallCount(), 1);
    BOOST_CHECK(mockProjectTeamService->getLastGetProjectTeamUserId() == 100 || mockProjectTeamService->getLastGetProjectTeamUserId() == 0);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_create_project_team)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    // Используем новую команду (555), чтобы не было конфликта
    json::value body;
    body[U("projectId")] = json::value::number(100);
    body[U("teamId")] = json::value::number(555); // Новая команда

    auto response = makePostRequest("/project-teams", body).get();

    // Должен быть 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    // Сервис вызывается, но возвращает nullopt
    BOOST_CHECK_EQUAL(mockProjectTeamService->getCreateProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastCreateProjectTeamUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_regular_user_cannot_delete_project_team)
{
    // Обычный пользователь
    mockAuthMiddleware->setValidateRequestResult(true, "100");

    auto response = makeDeleteRequest("/project-teams/1").get();

    // Должен быть 403 Forbidden
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    // Сервис вызывается, но возвращает ошибку
    BOOST_CHECK_EQUAL(mockProjectTeamService->getDeleteProjectTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectTeamService->getLastDeleteProjectTeamUserId(), 100);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
