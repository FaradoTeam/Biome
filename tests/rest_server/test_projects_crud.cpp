#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_phase_service.h"
#include "tests/server_mocks/mock_project_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;

namespace server
{
namespace tests
{

// Вспомогательная функция для URL-кодирования
std::string urlEncode(const std::string& value)
{
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (unsigned char c : value)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            escaped << c;
        }
        else
        {
            escaped << '%' << std::setw(2) << int(c);
        }
    }

    return escaped.str();
}

struct ProjectsTestFixture
{
    ProjectsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockPhaseService = std::make_shared<MockPhaseService>();
        mockProjectService = std::make_shared<MockProjectService>();
        mockUserService = std::make_shared<MockUserService>();

        // Настройка аутентификации по умолчанию (пользователь с ID 100)
        mockAuthMiddleware->setValidateRequestResult(true, "100");

        // Настройка тестовых данных
        setupDefaultProjectService();

        server = std::make_unique<RestServer>("127.0.0.1", 18084);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setPhaseService(mockPhaseService);
        server->setProjectService(mockProjectService);
        server->setUserService(mockUserService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread(
            [this]()
            {
                server->start();
            }
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void setupDefaultProjectService()
    {
        // Создаем список всех проектов
        auto project1 = MockProjectService::createTestProject(1, "Проект 1", std::nullopt, false);
        auto project2 = MockProjectService::createTestProject(2, "Проект 2", 1, false);
        auto project3 = MockProjectService::createTestProject(3, "Архивный проект", std::nullopt, true);
        auto project4 = MockProjectService::createTestProject(4, "Дочерний проект 2", 1, false);
        auto project5 = MockProjectService::createTestProject(5, "Корневой проект 2", std::nullopt, false);

        std::vector<dto::Project> allProjects = { project1, project2, project3, project4, project5 };

        // Устанавливаем все проекты для пагинации и фильтрации
        mockProjectService->setAllProjects(allProjects);

        // Настраиваем доступные проекты для разных пользователей
        mockProjectService->setUserAccessibleProjects(100, { 1, 2, 3, 4, 5 });
        mockProjectService->setUserAccessibleProjects(200, { 1, 2 });
        mockProjectService->setUserAccessibleProjects(999, {}); // Нет доступа

        // Для одиночных запросов project()
        mockProjectService->setGetProjectCallback(
            [](int64_t id, int64_t userId) -> std::optional<dto::Project>
            {
                // Супер-админ (ID 1) видит всё
                if (userId == 1)
                {
                    return MockProjectService::createTestProject(id, "Проект " + std::to_string(id));
                }
                // Пользователь 100 видит проекты 1-5
                if (userId == 100 && id >= 1 && id <= 5)
                {
                    return MockProjectService::createTestProject(id, "Проект " + std::to_string(id));
                }
                // Пользователь 200 видит только проекты 1 и 2
                if (userId == 200 && (id == 1 || id == 2))
                {
                    return MockProjectService::createTestProject(id, "Проект " + std::to_string(id));
                }
                // Пользователь 999 не видит ничего (возвращаем nullopt, но это будет 404, а не 403)
                // Для 403 нужно, чтобы проект существовал, но не было прав
                // Поэтому для пользователя 999 возвращаем nullopt (проект не существует)
                return std::nullopt;
            }
        );

        // Для создания проектов
        mockProjectService->setCreateProjectCallback(
            [](const dto::Project& project, int64_t userId) -> std::optional<dto::Project>
            {
                // Пользователь 999 не имеет прав на создание
                if (userId == 999)
                {
                    return std::nullopt;
                }
                auto newProject = MockProjectService::createTestProject(100, *project.caption);
                return newProject;
            }
        );

        // Для обновления проектов
        mockProjectService->setUpdateProjectCallback(
            [](const dto::Project& project, int64_t userId) -> std::optional<dto::Project>
            {
                // Если проект не существует (id вне диапазона)
                if (!project.id.has_value() || *project.id < 1 || *project.id > 5)
                {
                    return std::nullopt; // 404
                }
                // Пользователь 999 не имеет прав на обновление
                if (userId == 999)
                {
                    return std::nullopt; // 403 (проект существует, но нет прав)
                }
                // Пользователь 200 может обновлять только проект 1
                if (userId == 200 && *project.id != 1)
                {
                    return std::nullopt; // 403
                }
                return project;
            }
        );

        // Для архивации проектов
        mockProjectService->setArchiveProjectCallback(
            [](int64_t id, int64_t userId) -> bool
            {
                // Если проект не существует
                if (id < 1 || id > 5)
                {
                    return false; // 404
                }
                // Пользователь 999 не имеет прав
                if (userId == 999)
                {
                    return false; // 403
                }
                // Пользователь 200 может архивировать только проект 2
                if (userId == 200 && id != 2)
                {
                    return false; // 403
                }
                return true;
            }
        );

        // Для восстановления проектов
        mockProjectService->setRestoreProjectCallback(
            [](int64_t id, int64_t userId) -> bool
            {
                if (userId == 999)
                {
                    return false;
                }
                return true;
            }
        );
    }

    ~ProjectsTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    // Вспомогательные методы для отправки запросов
    pplx::task<web::http::http_response> makeGetRequest(
        const std::string& path,
        const std::string& token = "valid_token"
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18084"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18084"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18084"));
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
        web::http::client::http_client client(U("http://127.0.0.1:18084"));
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
    std::shared_ptr<MockProjectService> mockProjectService;
    std::shared_ptr<MockUserService> mockUserService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(ProjectsCrudTestSuite, ProjectsTestFixture)

// ============================================================
// GET /api/v1/projects — Получение списка проектов
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_projects_returns_list)
{
    auto response = makeGetRequest("/api/v1/projects").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->getProjectsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectsUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_projects_with_pagination_params)
{
    auto response = makeGetRequest("/api/v1/projects?page=2&pageSize=2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectsPage(), 2);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectsPageSize(), 2);

    auto json = response.extract_json().get();
    // На второй странице при pageSize=2 должны быть проекты 3 и 4
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 5);
}

BOOST_AUTO_TEST_CASE(test_get_projects_filter_by_parent)
{
    auto response = makeGetRequest("/api/v1/projects?parentId=1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockProjectService->getLastGetProjectsParentId().has_value());
    BOOST_CHECK_EQUAL(*mockProjectService->getLastGetProjectsParentId(), 1);
}

BOOST_AUTO_TEST_CASE(test_get_projects_filter_by_archive)
{
    auto response = makeGetRequest("/api/v1/projects?isArchive=true").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_REQUIRE(mockProjectService->getLastGetProjectsIsArchive().has_value());
    BOOST_CHECK(mockProjectService->getLastGetProjectsIsArchive().value());
}

BOOST_AUTO_TEST_CASE(test_get_projects_search_by_caption)
{
    mockProjectService->setUserAccessibleProjects(100, { 1, 2, 3, 4, 5 });

    // Используем английские названия
    auto project1 = MockProjectService::createTestProject(1, "Alpha Project", std::nullopt, false);
    auto project2 = MockProjectService::createTestProject(2, "Beta Project", std::nullopt, false);
    auto project3 = MockProjectService::createTestProject(3, "Gamma Version", std::nullopt, false);
    auto project4 = MockProjectService::createTestProject(4, "Project Delta", std::nullopt, false);
    auto project5 = MockProjectService::createTestProject(5, "Omega Final", std::nullopt, false);

    mockProjectService->setAllProjects({ project1, project2, project3, project4, project5 });

    // Ищем слово "Project"
    auto response = makeGetRequest("/api/v1/projects?searchCaption=Project").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    // Должен найти "Alpha Project", "Beta Project" и "Project Delta" (3 проекта)
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 3);

    // Дополнительная проверка: убеждаемся, что найдены правильные проекты
    auto items = json.at(U("items")).as_array();
    std::vector<std::string> foundCaptions;
    for (const auto& item : items)
    {
        std::string caption = utility::conversions::to_utf8string(item.at(U("caption")).as_string());
        foundCaptions.push_back(caption);
    }

    BOOST_CHECK(std::find(foundCaptions.begin(), foundCaptions.end(), "Alpha Project") != foundCaptions.end());
    BOOST_CHECK(std::find(foundCaptions.begin(), foundCaptions.end(), "Beta Project") != foundCaptions.end());
    BOOST_CHECK(std::find(foundCaptions.begin(), foundCaptions.end(), "Project Delta") != foundCaptions.end());
    BOOST_CHECK(std::find(foundCaptions.begin(), foundCaptions.end(), "Gamma Version") == foundCaptions.end());
    BOOST_CHECK(std::find(foundCaptions.begin(), foundCaptions.end(), "Omega Final") == foundCaptions.end());
}

BOOST_AUTO_TEST_CASE(test_get_projects_empty_list)
{
    // Временно меняем доступные проекты для пользователя 100 на пустые
    mockProjectService->setUserAccessibleProjects(100, {});
    mockProjectService->setAllProjects({});

    auto response = makeGetRequest("/api/v1/projects").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 0);
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 0);

    // Восстанавливаем
    setupDefaultProjectService();
}

BOOST_AUTO_TEST_CASE(test_get_projects_requires_auth)
{
    auto response = makeGetRequest("/api/v1/projects", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockProjectService->getProjectsCallCount(), 0);
}

// ============================================================
// GET /api/v1/projects/{id} — Получение проекта по ID
// ============================================================

BOOST_AUTO_TEST_CASE(test_get_project_by_id_success)
{
    auto response = makeGetRequest("/api/v1/projects/1").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->getProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectId(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectUserId(), 100);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Проект 1"));
}

BOOST_AUTO_TEST_CASE(test_get_project_not_found)
{
    auto response = makeGetRequest("/api/v1/projects/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
    BOOST_CHECK_EQUAL(mockProjectService->getProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectId(), 999);
}

BOOST_AUTO_TEST_CASE(test_get_project_invalid_id)
{
    auto response = makeGetRequest("/api/v1/projects/invalid").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_get_project_forbidden)
{
    // Пользователь с ID 200 пытается получить проект 3 (у него нет прав)
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    auto response = makeGetRequest("/api/v1/projects/3").get();

    // Должен быть 404, чтобы не раскрывать существование проекта
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// POST /api/v1/projects — Создание проекта
// ============================================================

BOOST_AUTO_TEST_CASE(test_create_project_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый проект"));
    body[U("description")] = web::json::value::string(U("Описание нового проекта"));

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockProjectService->createProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastCreateProjectUserId(), 100);
    BOOST_CHECK_EQUAL(*mockProjectService->getLastCreatedProject().caption, "Новый проект");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 100);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Новый проект"));
}

BOOST_AUTO_TEST_CASE(test_create_root_project_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Корневой проект"));

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockProjectService->createProjectCallCount(), 1);
    BOOST_CHECK(!mockProjectService->getLastCreatedProject().parentId.has_value());
}

BOOST_AUTO_TEST_CASE(test_create_subproject_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Дочерний проект"));
    body[U("parentId")] = web::json::value::number(1);

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockProjectService->createProjectCallCount(), 1);
    BOOST_REQUIRE(mockProjectService->getLastCreatedProject().parentId.has_value());
    BOOST_CHECK_EQUAL(*mockProjectService->getLastCreatedProject().parentId, 1);
}

BOOST_AUTO_TEST_CASE(test_create_project_missing_caption)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Проект без названия"));

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockProjectService->createProjectCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_project_empty_caption)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U(""));
    body[U("description")] = web::json::value::string(U("Пустое название"));

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockProjectService->createProjectCallCount(), 0);
}

BOOST_AUTO_TEST_CASE(test_create_project_forbidden)
{
    // Пользователь с ID 999 не имеет прав на создание
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Новый проект"));

    auto response = makePostRequest("/api/v1/projects", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("code")).as_integer(), 403);
}

// ============================================================
// PUT /api/v1/projects/{id} — Обновление проекта
// ============================================================

BOOST_AUTO_TEST_CASE(test_update_project_success)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновленный проект"));
    body[U("description")] = web::json::value::string(U("Новое описание"));

    auto response = makePutRequest("/api/v1/projects/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->updateProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastUpdateProjectUserId(), 100);
    BOOST_REQUIRE(mockProjectService->getLastUpdatedProject().id.has_value());
    BOOST_CHECK_EQUAL(*mockProjectService->getLastUpdatedProject().id, 1);
    BOOST_CHECK_EQUAL(*mockProjectService->getLastUpdatedProject().caption, "Обновленный проект");

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 1);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Обновленный проект"));
}

BOOST_AUTO_TEST_CASE(test_update_project_partial)
{
    web::json::value body;
    body[U("description")] = web::json::value::string(U("Только новое описание"));

    auto response = makePutRequest("/api/v1/projects/1", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->updateProjectCallCount(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_project_not_found)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Несуществующий"));

    auto response = makePutRequest("/api/v1/projects/999", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_update_project_forbidden)
{
    // Пользователь с ID 999 не имеет прав
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    // Важно: проект СУЩЕСТВУЕТ, но у пользователя нет прав
    // Настраиваем мок так, чтобы project() возвращал проект (означает, что проект существует)
    // но updateProject() возвращал nullopt (означает, что нет прав на обновление)

    mockProjectService->setGetProjectCallback(
        [](int64_t id, int64_t userId) -> std::optional<dto::Project>
        {
            // Проект существует для всех пользователей
            if (id == 1)
            {
                return MockProjectService::createTestProject(1, "Существующий проект");
            }
            return std::nullopt;
        }
    );

    mockProjectService->setUpdateProjectCallback(
        [](const dto::Project& project, int64_t userId) -> std::optional<dto::Project>
        {
            // Пользователь 999 не имеет прав на обновление
            if (userId == 999)
            {
                return std::nullopt; // Нет прав -> 403
            }
            return project;
        }
    );

    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Попытка взлома"));

    auto response = makePutRequest("/api/v1/projects/1", body).get();

    // Хэндлер должен вернуть 403, потому что:
    // 1. Проект существует (getProject вернул проект)
    // 2. Но updateProject вернул nullopt (нет прав)
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("code")).as_integer(), 403);
}

BOOST_AUTO_TEST_CASE(test_update_project_invalid_id)
{
    web::json::value body;
    body[U("caption")] = web::json::value::string(U("Обновление"));

    auto response = makePutRequest("/api/v1/projects/invalid", body).get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// ============================================================
// DELETE /api/v1/projects/{id} — Архивирование проекта
// ============================================================

BOOST_AUTO_TEST_CASE(test_archive_project_success)
{
    auto response = makeDeleteRequest("/api/v1/projects/2").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockProjectService->archiveProjectCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastArchivedProjectId(), 2);
    BOOST_CHECK_EQUAL(mockProjectService->getLastArchiveProjectUserId(), 100);
}

BOOST_AUTO_TEST_CASE(test_archive_project_not_found)
{
    auto response = makeDeleteRequest("/api/v1/projects/999").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_archive_project_forbidden)
{
    // Пользователь с ID 999 не имеет прав
    mockAuthMiddleware->setValidateRequestResult(true, "999");

    // Проект существует, но у пользователя нет прав на архивацию
    mockProjectService->setGetProjectCallback(
        [](int64_t id, int64_t userId) -> std::optional<dto::Project>
        {
            // Проект существует для всех пользователей
            if (id == 1)
            {
                return MockProjectService::createTestProject(1, "Существующий проект");
            }
            return std::nullopt;
        }
    );

    mockProjectService->setArchiveProjectCallback(
        [](int64_t id, int64_t userId) -> bool
        {
            // Пользователь 999 не имеет прав на архивацию
            if (userId == 999)
            {
                return false; // Нет прав -> 403
            }
            return true;
        }
    );

    auto response = makeDeleteRequest("/api/v1/projects/1").get();

    // Хэндлер должен вернуть 403
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Forbidden);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("code")).as_integer(), 403);
}

BOOST_AUTO_TEST_CASE(test_archive_project_invalid_id)
{
    auto response = makeDeleteRequest("/api/v1/projects/invalid").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_archive_project_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/projects/1", "").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockProjectService->archiveProjectCallCount(), 0);
}

// ============================================================
// Интеграционный тест (полный цикл)
// ============================================================

BOOST_AUTO_TEST_CASE(test_full_project_lifecycle)
{
    // 1. Создание проекта
    web::json::value createBody;
    createBody[U("caption")] = web::json::value::string(U("Жизненный цикл"));
    createBody[U("description")] = web::json::value::string(U("Тестовый проект"));

    auto createResponse = makePostRequest("/api/v1/projects", createBody).get();
    BOOST_CHECK_EQUAL(createResponse.status_code(), status_codes::Created);

    auto createJson = createResponse.extract_json().get();
    int64_t newProjectId = createJson.at(U("id")).as_integer();
    BOOST_CHECK_GT(newProjectId, 0);

    // Обновляем мок, чтобы новый проект был доступен
    mockProjectService->setGetProjectCallback(
        [newProjectId](int64_t id, int64_t userId) -> std::optional<dto::Project>
        {
            if (id == newProjectId && userId == 100)
            {
                return MockProjectService::createTestProject(newProjectId, "Жизненный цикл");
            }
            if (userId == 1) // Супер-админ
            {
                return MockProjectService::createTestProject(id, "Проект " + std::to_string(id));
            }
            if (userId == 100 && id >= 1 && id <= 5)
            {
                return MockProjectService::createTestProject(id, "Проект " + std::to_string(id));
            }
            return std::nullopt;
        }
    );

    // 2. Чтение созданного проекта
    auto getResponse = makeGetRequest("/api/v1/projects/" + std::to_string(newProjectId)).get();
    BOOST_CHECK_EQUAL(getResponse.status_code(), status_codes::OK);

    auto getJson = getResponse.extract_json().get();
    BOOST_CHECK_EQUAL(getJson.at(U("caption")).as_string(), U("Жизненный цикл"));

    // 3. Обновление проекта
    web::json::value updateBody;
    updateBody[U("caption")] = web::json::value::string(U("Обновленный цикл"));
    updateBody[U("description")] = web::json::value::string(U("Новое описание"));

    mockProjectService->setUpdateProjectCallback(
        [newProjectId](const dto::Project& project, int64_t userId) -> std::optional<dto::Project>
        {
            if (userId == 100 && project.id == newProjectId)
            {
                return project;
            }
            return std::nullopt;
        }
    );

    auto updateResponse = makePutRequest("/api/v1/projects/" + std::to_string(newProjectId), updateBody).get();
    BOOST_CHECK_EQUAL(updateResponse.status_code(), status_codes::OK);

    // 4. Архивирование проекта
    mockProjectService->setArchiveProjectCallback(
        [newProjectId](int64_t id, int64_t userId) -> bool
        {
            return (userId == 100 && id == newProjectId);
        }
    );

    auto archiveResponse = makeDeleteRequest("/api/v1/projects/" + std::to_string(newProjectId)).get();
    BOOST_CHECK_EQUAL(archiveResponse.status_code(), status_codes::NoContent);
}

// ============================================================
// Тесты проверки прав доступа
// ============================================================

BOOST_AUTO_TEST_CASE(test_access_denied_for_regular_user)
{
    // Обычный пользователь (ID 200) пытается получить доступ к проекту 3 (у него нет прав)
    mockAuthMiddleware->setValidateRequestResult(true, "200");

    auto response = makeGetRequest("/api/v1/projects/3").get();

    // Должен быть 404 (не раскрываем существование проекта)
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_super_admin_has_full_access)
{
    // Супер-администратор (ID 1) должен иметь доступ ко всем проектам
    mockAuthMiddleware->setValidateRequestResult(true, "1");

    auto response = makeGetRequest("/api/v1/projects").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockProjectService->getProjectsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockProjectService->getLastGetProjectsUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_invalid_token_returns_unauthorized)
{
    // Настраиваем middleware на отклонение неверного токена
    mockAuthMiddleware->setValidateRequestResult(false, "");

    auto response = makeGetRequest("/api/v1/projects", "invalid_token").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

BOOST_AUTO_TEST_CASE(test_expired_token_returns_unauthorized)
{
    // Настраиваем middleware на отклонение просроченного токена
    mockAuthMiddleware->setValidateRequestResult(false, "");

    auto response = makeGetRequest("/api/v1/projects", "expired_token").get();

    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace server
