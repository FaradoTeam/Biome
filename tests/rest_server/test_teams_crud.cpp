#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_team_service.h"

using namespace web;
using namespace web::http;
using namespace web::http::client;

namespace server::tests
{

struct TeamsTestFixture
{
    TeamsTestFixture()
    {
        mockAuthMiddleware = std::make_shared<MockAuthMiddleware>();
        mockAuthService = std::make_shared<MockAuthService>();
        mockTeamService = std::make_shared<MockTeamService>();

        // Используем супер-админа (userId=1)
        mockAuthMiddleware->setValidateRequestResult(true, "1");

        setupDefaultTeamService();

        server = std::make_unique<RestServer>("127.0.0.1", 18090);
        server->setAuthMiddleware(mockAuthMiddleware);
        server->setAuthService(mockAuthService);
        server->setTeamService(mockTeamService);

        BOOST_REQUIRE(server->initialize());

        serverThread = std::thread([this]()
                                   { server->start(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultTeamService()
    {
        services::TeamsPage testPage;
        dto::Team t1;
        t1.id = 1;
        t1.caption = "Team Alpha";
        dto::Team t2;
        t2.id = 2;
        t2.caption = "Team Beta";
        testPage.teams = { t1, t2 };
        testPage.totalCount = 2;
        mockTeamService->setGetTeamsResult(testPage);
        mockTeamService->setGetTeamResult(t1);

        dto::Team newTeam;
        newTeam.id = 10;
        newTeam.caption = "New Team";
        mockTeamService->setCreateTeamResult(newTeam);

        dto::Team updatedTeam = t1;
        updatedTeam.caption = "Updated Team";
        mockTeamService->setUpdateTeamResult(updatedTeam);
        mockTeamService->setDeleteTeamResult(true);
    }

    ~TeamsTestFixture()
    {
        if (server)
            server->stop();
        if (serverThread.joinable())
            serverThread.join();
    }

    pplx::task<http_response> makeGetRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18090"));
        http_request request(methods::GET);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    pplx::task<http_response> makePostRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18090"));
        http_request request(methods::POST);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makePutRequest(const std::string& path, const json::value& body, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18090"));
        http_request request(methods::PUT);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        request.set_body(body);
        return client.request(request);
    }

    pplx::task<http_response> makeDeleteRequest(const std::string& path, const std::string& token = "valid_token")
    {
        http_client client(U("http://127.0.0.1:18090"));
        http_request request(methods::DEL);
        request.set_request_uri(U(path));
        if (!token.empty())
            request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::unique_ptr<RestServer> server;
    std::shared_ptr<MockAuthMiddleware> mockAuthMiddleware;
    std::shared_ptr<MockAuthService> mockAuthService;
    std::shared_ptr<MockTeamService> mockTeamService;
    std::thread serverThread;
};

BOOST_FIXTURE_TEST_SUITE(TeamsCrudTestSuite, TeamsTestFixture)

// GET /api/v1/teams
BOOST_AUTO_TEST_CASE(test_get_teams_returns_list)
{
    auto response = makeGetRequest("/api/v1/teams").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamService->getGetTeamsCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamsUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    BOOST_CHECK_EQUAL(json.at(U("items")).as_array().size(), 2);
}

BOOST_AUTO_TEST_CASE(test_get_teams_with_pagination)
{
    services::TeamsPage emptyPage;
    mockTeamService->setGetTeamsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/teams?page=3&pageSize=10").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamsPage(), 3);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamsPageSize(), 10);
}

BOOST_AUTO_TEST_CASE(test_get_teams_with_search)
{
    services::TeamsPage emptyPage;
    mockTeamService->setGetTeamsResult(emptyPage);

    auto response = makeGetRequest("/api/v1/teams?searchCaption=search").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamsSearch(), "search");
}

// GET /api/v1/teams/{id}
BOOST_AUTO_TEST_CASE(test_get_team_by_id_success)
{
    dto::Team team;
    team.id = 5;
    team.caption = "Found Team";
    mockTeamService->setGetTeamResult(team);

    auto response = makeGetRequest("/api/v1/teams/5").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamId(), 5);
    BOOST_CHECK_EQUAL(mockTeamService->getLastGetTeamUserId(), 1);

    auto json = response.extract_json().get();
    BOOST_CHECK_EQUAL(json.at(U("id")).as_integer(), 5);
    BOOST_CHECK_EQUAL(json.at(U("caption")).as_string(), U("Found Team"));
}

BOOST_AUTO_TEST_CASE(test_get_team_not_found)
{
    mockTeamService->setGetTeamResult(std::nullopt);

    auto response = makeGetRequest("/api/v1/teams/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// POST /api/v1/teams
BOOST_AUTO_TEST_CASE(test_create_team_success)
{
    dto::Team created;
    created.id = 10;
    created.caption = "New Team";
    mockTeamService->setCreateTeamResult(created);

    json::value body;
    body[U("caption")] = json::value::string(U("New Team"));

    auto response = makePostRequest("/api/v1/teams", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Created);
    BOOST_CHECK_EQUAL(mockTeamService->getCreateTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamService->getLastCreatedTeam().caption.value_or(""), "New Team");
    BOOST_CHECK_EQUAL(mockTeamService->getLastCreateTeamUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_create_team_missing_caption)
{
    json::value body;
    body[U("description")] = json::value::string(U("desc"));

    auto response = makePostRequest("/api/v1/teams", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::BadRequest);
    BOOST_CHECK_EQUAL(mockTeamService->getCreateTeamCallCount(), 0);
}

// PUT /api/v1/teams/{id}
BOOST_AUTO_TEST_CASE(test_update_team_success)
{
    dto::Team updated;
    updated.id = 1;
    updated.caption = "Updated Team";
    mockTeamService->setUpdateTeamResult(updated);

    json::value body;
    body[U("caption")] = json::value::string(U("Updated Team"));

    auto response = makePutRequest("/api/v1/teams/1", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);
    BOOST_CHECK_EQUAL(mockTeamService->getUpdateTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamService->getLastUpdatedTeam().id.value_or(0), 1);
    BOOST_CHECK_EQUAL(mockTeamService->getLastUpdateTeamUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_update_team_not_found)
{
    mockTeamService->setUpdateTeamResult(std::nullopt);

    json::value body;
    body[U("caption")] = json::value::string(U("Ghost"));

    auto response = makePutRequest("/api/v1/teams/999", body).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

// DELETE /api/v1/teams/{id}
BOOST_AUTO_TEST_CASE(test_delete_team_success)
{
    mockTeamService->setDeleteTeamResult(true);

    auto response = makeDeleteRequest("/api/v1/teams/2").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NoContent);
    BOOST_CHECK_EQUAL(mockTeamService->getDeleteTeamCallCount(), 1);
    BOOST_CHECK_EQUAL(mockTeamService->getLastDeletedTeamId(), 2);
    BOOST_CHECK_EQUAL(mockTeamService->getLastDeleteTeamUserId(), 1);
}

BOOST_AUTO_TEST_CASE(test_delete_team_not_found)
{
    mockTeamService->setDeleteTeamResult(false);

    auto response = makeDeleteRequest("/api/v1/teams/999").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::NotFound);
}

BOOST_AUTO_TEST_CASE(test_delete_team_requires_auth)
{
    auto response = makeDeleteRequest("/api/v1/teams/1", "").get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    BOOST_CHECK_EQUAL(mockTeamService->getDeleteTeamCallCount(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace server::tests
