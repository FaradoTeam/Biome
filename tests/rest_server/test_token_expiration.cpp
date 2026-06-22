#include <chrono>
#include <thread>

#include <boost/test/unit_test.hpp>

#include <cpprest/http_client.h>

#include "api/middleware/impl/auth_middleware.h"
#include "api/rest_server.h"

#include "tests/server_mocks/mock_auth_middleware.h"
#include "tests/server_mocks/mock_auth_service.h"
#include "tests/server_mocks/mock_phase_service.h"
#include "tests/server_mocks/mock_project_service.h"
#include "tests/server_mocks/mock_user_service.h"

using namespace web;
using namespace web::http;
using namespace server;

namespace server
{
namespace tests
{

// Используем порт 18082, чтобы не конфликтовать с другими тестами
const std::string TEST_HOST = "127.0.0.1";
const uint16_t TEST_PORT = 18082;
const std::string TEST_SECRET_KEY = "your-very-long-secret-key-that-is-at-least-32-bytes-long!";

/**
 * @brief Фикстура для тестирования истечения токенов
 */
struct TokenExpirationFixture
{
    TokenExpirationFixture()
    {
        // Создаём реальный AuthMiddleware (нам нужна реальная работа с JWT)
        m_authMiddleware = std::make_shared<AuthMiddleware>(TEST_SECRET_KEY);

        // Создаём моки для сервисов
        m_mockAuthService = std::make_shared<MockAuthService>();
        m_mockPhaseService = std::make_shared<MockPhaseService>();
        m_mockProjectService = std::make_shared<MockProjectService>();
        m_mockUserService = std::make_shared<MockUserService>();

        // Настраиваем UserService с тестовыми данными
        setupDefaultUserService();

        // Создаём и настраиваем сервер
        m_server = std::make_shared<RestServer>(TEST_HOST, TEST_PORT);
        m_server->setAuthMiddleware(m_authMiddleware);
        m_server->setAuthService(m_mockAuthService);
        m_server->setPhaseService(m_mockPhaseService);
        m_server->setProjectService(m_mockProjectService);
        m_server->setUserService(m_mockUserService);

        BOOST_REQUIRE(m_server->initialize());
        m_server->start();

        // Даем серверу время на полный запуск
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    void setupDefaultUserService()
    {
        services::UsersPage testPage;

        dto::User user1;
        user1.id = 1;
        user1.login = "admin";
        user1.email = "admin@example.com";
        user1.firstName = "Admin";
        user1.lastName = "User";
        user1.isSuperAdmin = true;

        dto::User user2;
        user2.id = 2;
        user2.login = "test_user";
        user2.email = "test@example.com";
        user2.firstName = "Test";
        user2.lastName = "User";
        user2.isSuperAdmin = false;

        testPage.users = { user1, user2 };
        testPage.totalCount = 2;

        m_mockUserService->setGetUsersResult(testPage);
        m_mockUserService->setGetUserResult(user1);

        dto::User newUser;
        newUser.id = 3;
        newUser.login = "newuser";
        newUser.email = "new@test.com";
        m_mockUserService->setCreateUserResult(newUser);

        m_mockUserService->setUpdateUserResult(user1);
        m_mockUserService->setDeleteUserResult(true);
    }

    ~TokenExpirationFixture()
    {
        if (m_server)
        {
            m_server->stop();
        }
    }

    std::string generateShortLivedToken(int ttlSeconds = 2)
    {
        return m_authMiddleware->generateToken("100", ttlSeconds);
    }

    pplx::task<web::http::http_response> makeAuthenticatedRequest(
        const std::string& path,
        const std::string& token,
        const web::http::method& method = methods::GET
    )
    {
        web::http::client::http_client client(U("http://127.0.0.1:18082"));
        web::http::http_request request(method);
        request.set_request_uri(U(path));
        request.headers().add(U("Authorization"), U("Bearer " + token));
        return client.request(request);
    }

    std::shared_ptr<RestServer> m_server;
    std::shared_ptr<AuthMiddleware> m_authMiddleware;
    std::shared_ptr<MockAuthService> m_mockAuthService;
    std::shared_ptr<MockPhaseService> m_mockPhaseService;
    std::shared_ptr<MockProjectService> m_mockProjectService;
    std::shared_ptr<MockUserService> m_mockUserService;
};

/**
 * @brief Проверка, что токен перестает работать после истечения TTL
 */
BOOST_FIXTURE_TEST_CASE(test_token_expiration, TokenExpirationFixture)
{
    // Генерируем токен с коротким сроком действия (2 секунды)
    std::string shortLivedToken = generateShortLivedToken(2);

    // Сразу после генерации токен должен быть валидным
    {
        auto response = makeAuthenticatedRequest("/api/v1/users", shortLivedToken).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

        auto json = response.extract_json().get();
        BOOST_CHECK(json.has_field(U("items")));
        BOOST_CHECK(json.has_field(U("totalCount")));
        BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
    }

    // Ожидаем истечения срока действия токена
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Токен больше не должен работать — проверяем что сервер возвращает 401
    {
        auto response = makeAuthenticatedRequest("/api/v1/users", shortLivedToken).get();
        BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
    }
}

/**
 * @brief Проверка, что валидный токен продолжает работать
 */
BOOST_FIXTURE_TEST_CASE(test_valid_token_works, TokenExpirationFixture)
{
    // Генерируем токен с нормальным сроком действия (1 час)
    std::string validToken = generateShortLivedToken(3600);

    // Токен должен работать
    auto response = makeAuthenticatedRequest("/api/v1/users", validToken).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::OK);

    auto json = response.extract_json().get();
    BOOST_CHECK(json.has_field(U("items")));
    BOOST_CHECK(json.has_field(U("totalCount")));
    BOOST_CHECK_EQUAL(json.at(U("totalCount")).as_integer(), 2);
}

/**
 * @brief Проверка, что невалидный токен отвергается
 */
BOOST_FIXTURE_TEST_CASE(test_invalid_token_rejected, TokenExpirationFixture)
{
    std::string invalidToken = "this.is.not.a.valid.jwt.token";

    auto response = makeAuthenticatedRequest("/api/v1/users", invalidToken).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

/**
 * @brief Проверка, что аннулированный токен не работает
 */
BOOST_FIXTURE_TEST_CASE(test_invalidated_token_rejected, TokenExpirationFixture)
{
    // Генерируем токен
    std::string token = generateShortLivedToken(3600);

    // Аннулируем его
    m_authMiddleware->invalidateToken(token);

    // Теперь он не должен работать
    auto response = makeAuthenticatedRequest("/api/v1/users", token).get();
    BOOST_CHECK_EQUAL(response.status_code(), status_codes::Unauthorized);
}

} // namespace tests
} // namespace server
