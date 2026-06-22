#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_phase_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

struct PhasesTestFixture
{
    PhasesTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockPhaseService = std::make_shared<MockPhaseService>();
        mockUserService = std::make_shared<MockUserService>();

        mockAuthMiddleware->setValidateRequestResult(true, "100");

        setupDefaultPhaseService();

        server = std::make_unique<RestServer>("127.0.0.1", 18085);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setPhaseService(mockPhaseService);
        server->setUserService(mockUserService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultPhaseService()
    {
        // Создаём тестовые фазы
        dto::Phase phase1 = MockPhaseService::createTestPhase(1, "Фаза 1", 10, false);
        dto::Phase phase2 = MockPhaseService::createTestPhase(2, "Фаза 2", 10, false);
        dto::Phase phase3 = MockPhaseService::createTestPhase(3, "Архивная фаза", 10, true);
        dto::Phase phase4 = MockPhaseService::createTestPhase(4, "Фаза проекта 20", 20, false);

        std::vector<dto::Phase> allPhases = { phase1, phase2, phase3, phase4 };
        mockPhaseService->setAllPhases(allPhases);

        // Настраиваем доступные проекты для разных пользователей
        mockPhaseService->setUserAccessibleProjects(100, { 10, 20 });
        mockPhaseService->setUserAccessibleProjects(200, { 10 });
        mockPhaseService->setUserAccessibleProjects(999, {});

        // НЕ устанавливаем отдельные callback'и, так как filterPhases уже используется в phases()
        // Просто устанавливаем, что нужно использовать filterPhases через callback
        mockPhaseService->setGetPhasesCallback(
            [this](int page, int pageSize, int64_t userId, std::optional<int64_t> projectId, std::optional<bool> isArchive) -> services::PhasesPage
            {
                return mockPhaseService->filterPhases(page, pageSize, userId, projectId, isArchive);
            }
        );

        // Для остальных методов используем внутренние реализации
        mockPhaseService->setGetPhaseCallback(
            [this](int64_t id, int64_t userId) -> std::optional<dto::Phase>
            {
                return mockPhaseService->getPhaseById(id, userId);
            }
        );

        mockPhaseService->setCreatePhaseCallback(
            [this](const dto::Phase& phase, int64_t userId) -> std::optional<dto::Phase>
            {
                return mockPhaseService->createPhaseInternal(phase, userId);
            }
        );

        mockPhaseService->setUpdatePhaseCallback(
            [this](const dto::Phase& phase, int64_t userId) -> std::optional<dto::Phase>
            {
                return mockPhaseService->updatePhaseInternal(phase, userId);
            }
        );

        mockPhaseService->setArchivePhaseCallback(
            [this](int64_t id, int64_t userId) -> bool
            {
                return mockPhaseService->archivePhaseInternal(id, userId);
            }
        );

        mockPhaseService->setRestorePhaseCallback(
            [this](int64_t id, int64_t userId) -> bool
            {
                return mockPhaseService->restorePhaseInternal(id, userId);
            }
        );
    }

    ~PhasesTestFixture()
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18085"));
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
    std::shared_ptr<MockPhaseService> mockPhaseService;
    std::shared_ptr<MockUserService> mockUserService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(PhasesCrudTestSuite, PhasesTestFixture)

// ============================================================
// GET /api/v1/phases
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_phases_returns_list)
{
    auto response = makeGetRequest("/api/v1/phases").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPhaseService->getGetPhasesCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastGetPhasesUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    // Пользователь 100 видит: Фаза 1 (проект 10), Фаза 2 (проект 10), Фаза 4 (проект 20) = 3 фазы
    // Архивная фаза 3 не включается, так как isArchive по умолчанию false
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);
}

BOOST_AUTO_TEST_CASE(test_get_phases_with_pagination)
{
    auto response = makeGetRequest("/api/v1/phases?page=2&pageSize=1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastGetPhasesPage(), 2);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastGetPhasesPageSize(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("page")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("pageSize")).as_integer(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_phases_filtered_by_project)
{
    auto response = makeGetRequest("/api/v1/phases?projectId=10").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPhaseService->getLastGetPhasesProjectId().has_value());
    BOOST_CHECK_EQUAL(*mockPhaseService->getLastGetPhasesProjectId(), 10);

    auto json = response.extract_json().get();
    auto items = json.at(U("items")).as_array();

    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(item.at(U("projectId")).as_integer(), 10);
    }
}

BOOST_AUTO_TEST_CASE(test_get_phases_filtered_by_archive)
{
    auto response = makeGetRequest("/api/v1/phases?isArchive=true").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockPhaseService->getLastGetPhasesIsArchive().has_value());
    BOOST_CHECK(mockPhaseService->getLastGetPhasesIsArchive().value());

    auto json = response.extract_json().get();
    auto items = json.at(U("items")).as_array();

    for (const auto& item : items)
    {
        BOOST_CHECK(item.at(U("isArchive")).as_bool());
    }
}

BOOST_AUTO_TEST_CASE(test_get_phases_user_with_limited_access)
{
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    auto response = makeGetRequest("/api/v1/phases").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    // Пользователь 200 видит только: Фаза 1, Фаза 2 = 2 фазы
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);

    auto items = json.at(U("items")).as_array();
    for (const auto& item : items)
    {
        BOOST_CHECK_EQUAL(item.at(U("projectId")).as_integer(), 10);
    }
}

BOOST_AUTO_TEST_CASE(test_get_phases_user_with_no_access)
{
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    auto response = makeGetRequest("/api/v1/phases").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
}

BOOST_AUTO_TEST_CASE(test_get_phases_super_admin_sees_all)
{
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    auto response = makeGetRequest("/api/v1/phases").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    // Супер-админ видит все 4 фазы
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 4);
}

BOOST_AUTO_TEST_CASE(test_get_phases_requires_auth)
{
    auto response = makeGetRequest("/api/v1/phases", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPhaseService->getGetPhasesCallCount(), 0);
}

// ============================================================
// GET /api/v1/phases/{id}
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_phase_by_id_success)
{
    auto response = makeGetRequest("/api/v1/phases/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPhaseService->getGetPhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastGetPhaseId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Фаза 1"));
    BOOST_CHECK_EQUAL(json.at(U("projectId")).as_integer(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_phase_not_found)
{
    auto response = makeGetRequest("/api/v1/phases/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPhaseService->getGetPhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastGetPhaseId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_phase_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    auto response = makeGetRequest("/api/v1/phases/4").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPhaseService->getGetPhaseCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_phase_invalid_id)
{
    auto response = makeGetRequest("/api/v1/phases/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/phases
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_phase_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новая фаза"));
    body[U("projectId")] = web::json::value::number(10);
    body[U("description")] = web::json::value::string(U("Описание новой фазы"));

    auto response = makePostRequest("/api/v1/phases", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockPhaseService->getCreatePhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastCreatePhaseUserId(), 100);
    BOOST_CHECK_EQUAL(*mockPhaseService->getLastCreatedPhase().caption, "Новая фаза");

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("id")));
    BOOST_CHECK(json.at(U("id")).as_integer() > 0);
}

BOOST_AUTO_TEST_CASE(test_create_phase_missing_caption)
{
    web::json::value body;
    body[U("projectId")] = web::json::value::number(10);

    auto response = makePostRequest("/api/v1/phases", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPhaseService->getCreatePhaseCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_phase_missing_project_id)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Фаза без проекта"));

    auto response = makePostRequest("/api/v1/phases", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockPhaseService->getCreatePhaseCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_phase_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новая фаза"));
    body[U("projectId")] = web::json::value::number(20);

    auto response = makePostRequest("/api/v1/phases", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);
    BOOST_CHECK_EQUAL(mockPhaseService->getCreatePhaseCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_phase_with_dates)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Фаза с датами"));
    body[U("projectId")] = web::json::value::number(10);
    body[U("beginDate")] = web::json::value::number(1640995200);
    body[U("endDate")] = web::json::value::number(1643673600);

    auto response = makePostRequest("/api/v1/phases", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("beginDate")));
    BOOST_CHECK(json.has_field(U("endDate")));
}

// ============================================================
// PUT /api/v1/phases/{id}
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_phase_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновлённая фаза"));
    body[U("description")] = web::json::value::string(U("Новое описание"));

    auto response = makePutRequest("/api/v1/phases/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPhaseService->getUpdatePhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(*mockPhaseService->getLastUpdatedPhase().id, 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновлённая фаза"));
}

BOOST_AUTO_TEST_CASE(test_update_phase_partial)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Только новое описание"));

    auto response = makePutRequest("/api/v1/phases/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockPhaseService->getUpdatePhaseCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_phase_not_found)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующая фаза"));

    auto response = makePutRequest("/api/v1/phases/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPhaseService->getUpdatePhaseCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_phase_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка обновления"));

    auto response = makePutRequest("/api/v1/phases/4", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockPhaseService->getUpdatePhaseCallCount(), 1);
}

// ============================================================
// DELETE /api/v1/phases/{id}
// ============================================================

BOOST_AUTO_TEST_CASE(test_archive_phase_success)
{
    auto response = makeDeleteRequest("/api/v1/phases/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockPhaseService->getArchivePhaseCallCount(), 1);
    BOOST_CHECK_EQUAL(mockPhaseService->getLastArchivedPhaseId(), 2);
}

BOOST_AUTO_TEST_CASE(test_archive_phase_not_found)
{
    auto response = makeDeleteRequest("/api/v1/phases/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    // archivePhaseInternal не вызывается, так как phase(999) вернёт nullopt
    BOOST_CHECK_EQUAL(mockPhaseService->getArchivePhaseCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_archive_phase_access_denied)
{
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    auto response = makeDeleteRequest("/api/v1/phases/4").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    // archivePhaseInternal не вызывается, так как у пользователя 200 нет доступа к проекту 20
    BOOST_CHECK_EQUAL(mockPhaseService->getArchivePhaseCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_archive_phase_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/phases/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockPhaseService->getArchivePhaseCallCount(), 0);
}

// ============================================================
// Интеграционный тест: полный жизненный цикл
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_phase_lifecycle)
{
    // 1. Создание
    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл фазы"));
    createBody[U("projectId")] = web::json::value::number(10);
    createBody[U("description")] = web::json::value::string(U("Тестовая фаза"));

    auto createResponse = makePostRequest("/api/v1/phases", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    auto createJson = createResponse.extract_json().get();
    int64_t newPhaseId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newPhaseId, 0);

    // 2. Чтение
    auto getResponse = makeGetRequest("/api/v1/phases/" + std::to_string(newPhaseId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    // 3. Обновление
    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновлённая фаза"));

    auto updateResponse = makePutRequest("/api/v1/phases/" + std::to_string(newPhaseId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Архивирование
    auto archiveResponse = makeDeleteRequest("/api/v1/phases/" + std::to_string(newPhaseId)).get();
    BOOST_CHECK_EQUAL(archiveResponse.status_code(), status_codes::NoContent);

    // 5. Проверка, что фаза в архиве
    getResponse = makeGetRequest("/api/v1/phases/" + std::to_string(newPhaseId)).get();
    auto getJson = getResponse.extract_json().get();
    BOOST_CHECK(getJson.at(U("isArchive")).as_bool());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
